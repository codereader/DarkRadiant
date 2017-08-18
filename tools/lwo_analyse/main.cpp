#include <vector>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

/**
 * greebo: This is a quick-n-dirty console application to parse the contents
 * of an LWO2 model file and dump its structure to standard output.
 * It has some routines to filter out certain Chunks from the file and 
 * write the modified file to disk for debugging purposes, but these
 * functions are unreachable code right now.
 */

template<typename ValueType>
inline ValueType reverse(ValueType value)
{
	ValueType output = value;
	std::reverse(reinterpret_cast<char*>(&output), reinterpret_cast<char*>(&output) + sizeof(ValueType));

	return output;
}

template<typename CharT, typename TraitsT = std::char_traits<CharT> >
class VectorBuffer : public std::basic_streambuf<CharT, TraitsT> {
public:
	VectorBuffer(std::vector<CharT> &vec) {
		setg(vec.data(), vec.data(), vec.data() + vec.size());
	}
};

typedef std::vector<char> CharVector;

struct Chunk
{
	std::string id;
	uint32_t size;
	CharVector rawData;
	uint16_t chunkSizeBytes;

	// stuff before subchunks
	CharVector contents;
	// subchunks with ID
	std::vector<Chunk> subChunks;

	unsigned int getSize()
	{
		unsigned int totalSize = 0;

		// Start with the size of the contents 
		// (don't use seek as we don't know if the client still wants to write stuff)
		totalSize += static_cast<unsigned int>(contents.size());

		if (!subChunks.empty())
		{
			// Sum up the size of the subchunks
			for (Chunk& chunk : subChunks)
			{
				totalSize += 4; // ID (4 bytes)
				totalSize += chunk.chunkSizeBytes; // Subchunk Size Info (can be 4 or 2 bytes)

				// While the child chunk size itself doesn't include padding, we need to respect
				// it when calculating the size of this parent chunk
				unsigned int childChunkSize = chunk.getSize();
				totalSize += childChunkSize + (childChunkSize % 2); // add 1 padding byte if odd
			}
		}
		else
		{
			totalSize += rawData.size();
		}

		return totalSize;
	}
};

std::string parseString0(std::istream& stream)
{
	std::vector<char> str;
	char c;

	stream.read(&c, 1);
	str.push_back(c);

	while (c != '\0')
	{
		stream.read(&c, 1);
		str.push_back(c);
	}

	// optional 0 padding byte
	if (str.size() % 2 == 1)
	{
		stream.read(&c, 1);
	}

	return std::string(&(str.front()));
}

void addStringToContents(CharVector& contents, const std::string& str)
{
	std::size_t bytesWritten = 0;

	for (std::size_t i = 0; i < str.length(); ++i)
	{
		contents.push_back(str[i]);
		++bytesWritten;
	}

	contents.push_back('\0');
	++bytesWritten;

	if (bytesWritten % 2 == 1)
	{
		contents.push_back('\0');
		++bytesWritten;
	}
}

void parseFromStream(CharVector& buffer, int level, Chunk& parsedChunk)
{
	VectorBuffer<char> tempBuf(buffer);
	std::istream stream(&tempBuf);

	if (parsedChunk.id == "SURF")
	{
		std::string surfName = parseString0(stream);
		std::string parentName = parseString0(stream);

		addStringToContents(parsedChunk.contents, surfName);
		addStringToContents(parsedChunk.contents, parentName);
	}

	if (parsedChunk.id == "IMAP")
	{
		std::string ordinal = parseString0(stream);
		addStringToContents(parsedChunk.contents, ordinal);
	}

	while (!stream.eof())
	{
		// Read header
		Chunk chunk;

		char id[5];
		stream.read(id, 4);
		id[4] = '\0';

		chunk.id = id;

		if (stream.eof()) break;

		if (parsedChunk.id == "SURF" || parsedChunk.id == "BLOK" ||
			parsedChunk.id == "IMAP" || parsedChunk.id == "TMAP")
		{
			uint16_t size;
			stream.read((char*)&size, 2);
			size = reverse(size);
			chunk.size = size;
			chunk.chunkSizeBytes = 2;
		}
		else
		{
			uint32_t size;
			stream.read((char*)&size, 4);
			size = reverse(size);
			chunk.size = size;
			chunk.chunkSizeBytes = 4;
		}

		assert(!stream.fail());

		//std::string indent(level * 2, ' ');
		//std::cout << indent << id << " with size " << chunk.size << " bytes" << std::endl;

		// Check size restrictions
		assert(4 + chunk.chunkSizeBytes + chunk.size <= buffer.size());

		uint32_t dataSize = chunk.size;

		if (chunk.id == "FORM")
		{
			// LWO doesn't have the correct size?
			if (dataSize + 4 + 4 != buffer.size())
			{
				std::cout << "ERROR: FORM Size Value + 8 Bytes is not the same as the file size." << std::endl;
				assert(false);
			}

			// Read the LWO2 tag
			char lwo2[5];
			stream.read(lwo2, 4);
			lwo2[4] = '\0';

			assert(std::string(lwo2) == "LWO2");

			chunk.contents.push_back('L');
			chunk.contents.push_back('W');
			chunk.contents.push_back('O');
			chunk.contents.push_back('2');

			dataSize -= 4;
		}

		assert(!stream.fail());

		chunk.rawData.resize(dataSize);
		stream.read(&chunk.rawData.front(), dataSize);

#if 0
		if (chunk.id == "COLR")
		{
			uint32_t colr1Raw = *((uint32_t*)&chunk.rawData[0]);
			colr1Raw = reverse(colr1Raw);
			float colr1 = *((float*)&colr1Raw);
			std::cout << colr1;
		}
#endif

		assert(!stream.fail());

		// Fill bit if size is odd
		if (dataSize % 2 == 1)
		{
			char temp;
			stream.read(&temp, 1);
			assert(temp == '\0');
			assert(!stream.fail());
		}

		parsedChunk.subChunks.push_back(chunk);
	}

	// Try to parse the subchunks
	for (Chunk& chunk : parsedChunk.subChunks)
	{
		if (chunk.id == "FORM" || chunk.id == "SURF" || chunk.id == "BLOK" ||
			chunk.id == "IMAP" || chunk.id == "TMAP")
		{
			parseFromStream(chunk.rawData, level + 1, chunk);
		}
	}
}

void dumpChunks(Chunk& chunk, int level = 0)
{
	std::string indent(level * 2, ' ');
	std::cout << indent << chunk.id << " [" << chunk.size << "]" << std::endl;

	for (Chunk& subChunk : chunk.subChunks)
	{
		dumpChunks(subChunk, level + 1);
	}
}

void filterFile(Chunk& chunk)
{
	std::cout << "Filtering chunk " << chunk.id << std::endl;

	chunk.subChunks.erase(std::remove_if(chunk.subChunks.begin(), chunk.subChunks.end(), [](const Chunk& chunk)
		{
			return chunk.id == "VMAD" || chunk.id == "CLIP" || 
				chunk.id == "PIXB" || chunk.id == "AAST" || chunk.id == "WRPW" || chunk.id == "WRPH" || chunk.id == "WRAP" || chunk.id == "IMAG" ||
				chunk.id == "LUMI" || chunk.id == "DIFF" || chunk.id == "SPEC" || chunk.id == "REFL" || chunk.id == "TRAN" || chunk.id == "SMAN" || 
				chunk.id == "SIDE" || chunk.id == "GLOS" || chunk.id == "RIND" // || chunk.id == "COLR"
				;
		}), 
		chunk.subChunks.end());

	// Filter all subchunks
	for (Chunk& subChunk : chunk.subChunks)
	{
		filterFile(subChunk);
	}

	// Recalculate size after diving in
	chunk.size = chunk.getSize();
}

void writeFile(std::ofstream& stream, Chunk& chunk)
{
	std::cout << "Writing chunk " << chunk.id << std::endl;

	stream.write(chunk.id.c_str(), 4);

	if (chunk.chunkSizeBytes == 4)
	{
		uint32_t size = chunk.size;
		size = reverse(size);
		stream.write((char*)&size, 4);
	}
	else if (chunk.chunkSizeBytes == 2)
	{
		uint16_t size = chunk.size;
		size = reverse(size);
		stream.write((char*)&size, 2);
	}

	if (!chunk.contents.empty())
	{
		stream.write(&(chunk.contents.front()), chunk.contents.size());
	}

	if (chunk.subChunks.empty())
	{
		stream.write(&(chunk.rawData.front()), chunk.rawData.size());
	}
	else
	{
		for (Chunk& subChunk : chunk.subChunks)
		{
			writeFile(stream, subChunk);
		}
	}
}

void analyseLwo(const std::string& filename, bool modify)
{
	std::cout << "--- " << filename << " ---" << std::endl;

	std::ifstream istream(filename.c_str(), std::ios::binary);

	std::vector<char> vectorBuffer((std::istreambuf_iterator<char>(istream)), std::istreambuf_iterator<char>());

	Chunk form;
	form.id = "FORM";
	form.size = vectorBuffer.size();
	form.chunkSizeBytes = 4;

	parseFromStream(vectorBuffer, 0, form);

	if (!form.subChunks.empty())
	{
		dumpChunks(form.subChunks[0]);
	}

	istream.close();

	if (!modify) return;

	// Write modified LWO
	std::string modFile = filename.substr(0, filename.length() - 4) + "_modified.lwo";
	std::ofstream output(modFile.c_str(), std::ios::binary);

	if (!form.subChunks.empty())
	{
		filterFile(form.subChunks[0]);
		writeFile(output, form.subChunks[0]);
	}
}

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		std::cout << "Usage: LwoAnalyse <pathToLwoFile>";
		return 0;
	}

	std::string targetFile = argv[1];

	analyseLwo(targetFile, false);

    return 0;
}
