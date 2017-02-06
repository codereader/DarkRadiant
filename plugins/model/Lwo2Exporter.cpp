#include "Lwo2Exporter.h"

#include <vector>
#include "itextstream.h"
#include "imodelsurface.h"
#include "imap.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

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
	typedef boost::iostreams::back_insert_device<std::string> Device;

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
	std::vector<Chunk> subChunks;

private:
	Device _device;

public:
	boost::iostreams::stream<Device> stream;

	Chunk(const std::string& identifier_, Type type) :
		_chunkType(type),
		identifier(identifier_),
		_device(contents),
		stream(_device)
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
		_device(contents),
		stream(_device),
		subChunks(other.subChunks)
	{}

	// Returns the size of this Chunk's content
	// excluding this Chunk's ID (4 bytes) and Size info (4 bytes)
	unsigned int getContentSize() const
	{
		unsigned int totalSize = 0;

		// Start with the size of the contents
		totalSize += static_cast<unsigned int>(contents.size());

		if (!subChunks.empty())
		{
			// Sum up the size of the subchunks
			for (const Chunk& chunk : subChunks)
			{
				totalSize += 4; // ID (4 bytes)
				totalSize += _childChunkSizeBytes; // Subchunk Size Info (can be 4 or 2 bytes)
				totalSize += chunk.getContentSize(); // ID
			}
		}

		// Chunk size can be odd, padding must be handled by client code
		return totalSize;
	}

	// Adds the specified empty Chunk and returns its reference
	Chunk& addChunk(const std::string& identifier_, Type type)
	{
		subChunks.push_back(Chunk(identifier_, type));
		return subChunks.back();
	}
};

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
	fileChunk.stream << "LWO2";

	// Assemble the list of regular Chunks, these all use 4 bytes for size info

	// TAGS
	Chunk& tags = fileChunk.addChunk("TAGS", Chunk::Type::Chunk);
	
	tags.stream << '\0'; // empty string as first tag name

	// Create a single layer for the geometry
	Chunk& layr = fileChunk.addChunk("LAYR", Chunk::Type::Chunk);

	// LAYR{ number[U2], flags[U2], pivot[VEC12], name[S0], parent[U2] ? }

	layr.stream << uint16_t(0); // number[U2]
	layr.stream << uint16_t(0); // flags[U2]
	layr.stream << float(0) << float(0) << float(0); // pivot[VEC12]
	layr.stream << '\0'; // name[S0]
	// no parent index

	// PNTS
	Chunk& pnts = fileChunk.addChunk("PNTS", Chunk::Type::Chunk);

	// Load all vertex coordinates into this chunk


	rMessage() << "Buffer size is " << fileChunk.contents.size() << std::endl;
}

}
