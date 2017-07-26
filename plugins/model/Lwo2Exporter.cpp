#include "Lwo2Exporter.h"

#include <vector>
#include "itextstream.h"
#include "imodelsurface.h"
#include "imap.h"
#include "bytestreamutils.h"

namespace model
{

namespace
{

struct Chunk;

// Represents a Chunk in the LWO2 file, accepting content
// through the public stream member. 
// A Chunk can have contents and/or 1 or more subchunks. 
struct Chunk
{
public:
	typedef std::shared_ptr<Chunk> Ptr;

	enum class Type
	{
		Chunk,
		SubChunk
	};

private:
	Type _chunkType;

	// The number of bytes used for subchunks
	unsigned int _childChunkSizeBytes;

public:
	std::string identifier; // the 4-byte ID
	
	// The contents of this chunk (don't stream subchunk contents
	// into here, append them as subChunks instead)
	std::string contents;

	// Child chunks
	std::vector<Chunk::Ptr> subChunks;

public:
	//boost::iostreams::stream<Device> stream;
	std::stringstream stream;

	Chunk(const std::string& identifier_, Type type) :
		_chunkType(type),
		identifier(identifier_),
		stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary)
	{
		// FORM sub-chunks are normal chunks and have 4 bytes size info
		// whereas subchunks of e.g. CLIP use 2 bytes of size info
		_childChunkSizeBytes = _chunkType == Type::Chunk ? 4 : 2;
	}

	// Copy ctor
	Chunk(const Chunk& other) :
		_chunkType(other._chunkType),
		_childChunkSizeBytes(other._childChunkSizeBytes),
		identifier(other.identifier),
		stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary),
		subChunks(other.subChunks)
	{}

	// Returns the size of this Chunk's content
	// excluding this Chunk's ID (4 bytes) and Size info (4 bytes)
	unsigned int getContentSize() const
	{
		unsigned int totalSize = 0;

		// Start with the size of the contents 
		// (don't use seek as we don't know if the client still wants to write stuff)
		totalSize += static_cast<unsigned int>(stream.str().length());

		if (!subChunks.empty())
		{
			// Sum up the size of the subchunks
			for (const Chunk::Ptr& chunk : subChunks)
			{
				totalSize += 4; // ID (4 bytes)
				totalSize += _childChunkSizeBytes; // Subchunk Size Info (can be 4 or 2 bytes)

				// While the child chunk size itself doesn't include padding, we need to respect
				// it when calculating the size of this parent chunk
				unsigned int childChunkSize = chunk->getContentSize();
				totalSize += childChunkSize + (childChunkSize % 2); // add 1 padding byte if odd
			}
		}

		// Chunk size can be odd, padding must be handled by client code
		return totalSize;
	}

	// Adds the specified empty Chunk and returns its reference
	Chunk::Ptr addChunk(const std::string& identifier_, Type type)
	{
		subChunks.push_back(std::make_shared<Chunk>(identifier_, type));
		return subChunks.back();
	}

	void flushBuffer()
	{
		stream.flush();

		for (const Chunk::Ptr& chunk : subChunks)
		{
			chunk->flushBuffer();
		}
	}

	void writeToStream(std::ostream& output)
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
			stream::writeBigEndian<uint16_t>(output, getContentSize());
		}

		// Write the direct contents of this chunk
		stream.seekg(0, std::stringstream::beg);
		output << stream.rdbuf();

		// Write all subchunks
		for (const Chunk::Ptr& chunk : subChunks)
		{
			chunk->writeToStream(output);

			// Add the padding byte after the chunk
			if (chunk->getContentSize() % 2 == 1)
			{
				output.write("\0", 1);
			}
		}
	}
};

void writeVariableIndex(std::ostream& stream, std::size_t index)
{
	// LWO2 defines the variable index VX data type which is
	// 32 bit as soon as the index value is greater than 0xFF00, otherwise 16 bit
	if (index < 0xFF00)
	{
		stream::writeBigEndian<uint16_t>(stream, static_cast<uint16_t>(index));
	}
	else
	{
		// According to the specs, for values greater than 0xFF00:
		// "the index is written as an unsigned four byte integer with bits 24-31 set"
		stream::writeBigEndian<uint32_t>(stream, static_cast<uint32_t>(index) | 0xFF000000);
	}
}

}

Lwo2Exporter::Lwo2Exporter()
{}

IModelExporterPtr Lwo2Exporter::clone()
{
	return std::make_shared<Lwo2Exporter>();
}

const std::string& Lwo2Exporter::getExtension() const
{
	static std::string _extension("LWO");
	return _extension;
}

// Adds the given Surface to the exporter's queue
void Lwo2Exporter::addSurface(const IModelSurface& incoming)
{
	_surfaces.push_back(Surface());

	Surface& surface = _surfaces.back();
	surface.materialName = incoming.getDefaultMaterial();

	// Pull in all the triangles of that mesh
	for (int i = 0; i < incoming.getNumTriangles(); ++i)
	{
		ModelPolygon poly = incoming.getPolygon(i);

		unsigned int indexStart = static_cast<unsigned int>(surface.vertices.size());

		surface.vertices.push_back(poly.a);
		surface.vertices.push_back(poly.b);
		surface.vertices.push_back(poly.c);

		surface.indices.push_back(indexStart);
		surface.indices.push_back(indexStart + 1);
		surface.indices.push_back(indexStart + 2);
	}
}

// Export the model file to the given stream
void Lwo2Exporter::exportToStream(std::ostream& stream)
{
	Chunk fileChunk("FORM", Chunk::Type::Chunk);

	// The data of the FORM file contains just the LWO2 id and the collection of chunks
	fileChunk.stream.write("LWO2", 4);

	// Assemble the list of regular Chunks, these all use 4 bytes for size info

	// TAGS
	Chunk::Ptr tags = fileChunk.addChunk("TAGS", Chunk::Type::Chunk);
	
	// Export all material names as tags
	if (!_surfaces.empty())
	{
		for (const Surface& surface : _surfaces)
		{
			// Include the nul character at the end
			tags->stream.write(surface.materialName.c_str(), surface.materialName.length() + 1);
		}
	}
	else
	{
		tags->stream.write("\0", 1); // empty string as first tag name
	}

	// Create a single layer for the geometry
	Chunk::Ptr layr = fileChunk.addChunk("LAYR", Chunk::Type::Chunk);

	// LAYR{ number[U2], flags[U2], pivot[VEC12], name[S0], parent[U2] ? }

	stream::writeBigEndian<uint16_t>(layr->stream, 0); // number[U2]
	stream::writeBigEndian<uint16_t>(layr->stream, 0); // flags[U2]

	// pivot[VEC12]
	stream::writeBigEndian<float>(layr->stream, 0);
	stream::writeBigEndian<float>(layr->stream, 0);
	stream::writeBigEndian<float>(layr->stream, 0);

	layr->stream.write("\0", 1); // name[S0]
	// no parent index

	// Create the chunks for PNTS, POLS, PTAG
	Chunk::Ptr pnts = fileChunk.addChunk("PNTS", Chunk::Type::Chunk);
	Chunk::Ptr pols = fileChunk.addChunk("POLS", Chunk::Type::Chunk);
	Chunk::Ptr ptag = fileChunk.addChunk("PTAG", Chunk::Type::Chunk);

	// We only ever export FACE polygons
	pols->stream.write("FACE", 4);
	ptag->stream.write("SURF", 4); // we tag the surfaces

	// Load all vertex coordinates into this chunk
	for (std::size_t surfNum = 0; surfNum < _surfaces.size(); ++surfNum)
	{
		Surface& surface = _surfaces[surfNum];

		for (const ArbitraryMeshVertex& vertex : surface.vertices)
		{
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.x()));
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.y()));
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.z()));
		}

		int16_t numVerts = 3; // we export triangles

		for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3)
		{
			std::size_t polyNum = i / 3;

			stream::writeBigEndian<uint16_t>(pols->stream, numVerts); // [U2]

			// Fixme: Index type is VX, handle cases larger than FF00
			writeVariableIndex(pols->stream, surface.indices[i+0]); // [VX]
			writeVariableIndex(pols->stream, surface.indices[i+1]); // [VX]
			writeVariableIndex(pols->stream, surface.indices[i+2]); // [VX]

			// Fixme: Index type is VX, handle cases larger than FF00
			writeVariableIndex(ptag->stream, polyNum); // [VX]
			stream::writeBigEndian<uint16_t>(ptag->stream, static_cast<uint16_t>(surfNum)); // [U2]
		}

		// Write the SURF chunk for the surface
		Chunk::Ptr surf = fileChunk.addChunk("SURF", Chunk::Type::Chunk);

		if (!surface.materialName.empty())
		{
			surf->stream.write(surface.materialName.c_str(), surface.materialName.length() + 1);
		}
		else
		{
			surf->stream.write("\0", 1);
		}

		surf->stream.write("\0", 1); // empty parent name
	}

	fileChunk.writeToStream(stream);
}

}
