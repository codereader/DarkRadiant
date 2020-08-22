#include "Lwo2Chunk.h"

#include "stream/utils.h"

namespace model
{

Lwo2Chunk::Lwo2Chunk(const std::string& identifier_, Type type) :
	_chunkType(type),
	identifier(identifier_),
	stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary)
{
	// FORM sub-chunks are normal chunks and have 4 bytes size info
	// whereas subchunks of e.g. CLIP use 2 bytes of size info
	_sizeDescriptorByteCount = _chunkType == Type::Chunk ? 4 : 2;
}

unsigned int Lwo2Chunk::getContentSize() const
{
	unsigned int totalSize = 0;

	// Start with the size of the contents 
	// (don't use seek as we don't know if the client still wants to write stuff)
	totalSize += static_cast<unsigned int>(stream.str().length());

	if (!subChunks.empty())
	{
		// Sum up the size of the subchunks
		for (const Lwo2Chunk::Ptr& chunk : subChunks)
		{
			totalSize += 4; // ID (4 bytes)
			totalSize += chunk->_sizeDescriptorByteCount; // Subchunk Size Info (can be 4 or 2 bytes)

			// While the child chunk size itself doesn't include padding, we need to respect
			// it when calculating the size of this parent chunk
			unsigned int childChunkSize = chunk->getContentSize();
			totalSize += childChunkSize + (childChunkSize % 2); // add 1 padding byte if odd
		}
	}

	// Chunk size can be odd, padding must be handled by client code
	return totalSize;
}

Lwo2Chunk::Ptr Lwo2Chunk::addChunk(const std::string& identifier_, Type type)
{
	subChunks.push_back(std::make_shared<Lwo2Chunk>(identifier_, type));
	return subChunks.back();
}

Lwo2Chunk::Ptr Lwo2Chunk::addChunk(const std::string& identifier_)
{
	return addChunk(identifier_, Type::Chunk);
}

// Adds a Chunk (size type U2) to this one
Lwo2Chunk::Ptr Lwo2Chunk::addSubChunk(const std::string& identifier_)
{
	return addChunk(identifier_, Type::SubChunk);
}

void Lwo2Chunk::flushBuffer()
{
	stream.flush();

	for (const Lwo2Chunk::Ptr& chunk : subChunks)
	{
		chunk->flushBuffer();
	}
}

void Lwo2Chunk::writeToStream(std::ostream& output)
{
	// Flush all buffers before writing to the output stream
	flushBuffer();

	output.write(identifier.c_str(), identifier.length());

	if (_chunkType == Type::Chunk)
	{
		stream::writeBigEndian<uint32_t>(output, getContentSize());
	}
	else
	{
		stream::writeBigEndian<uint16_t>(output, static_cast<uint16_t>(getContentSize()));
	}

	// Write the direct contents of this chunk
	std::string str = stream.str();
	output.write(str.c_str(), str.length());

	// Write all subchunks
	for (const Lwo2Chunk::Ptr& chunk : subChunks)
	{
		chunk->writeToStream(output);

		// Add the padding byte after the chunk
		if (chunk->getContentSize() % 2 == 1)
		{
			output.write("\0", 1);
		}
	}
}

}
