#include "Lwo2Exporter.h"

#include <vector>
#include <iomanip>
#include "itextstream.h"
#include "imodelsurface.h"
#include "imap.h"
#include "math/AABB.h"
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

	// The number of bytes used for the size info of this chunk
	unsigned int _sizeDescriptorByteCount;

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
		_sizeDescriptorByteCount = _chunkType == Type::Chunk ? 4 : 2;
	}

	// Copy ctor
	Chunk(const Chunk& other) :
		_chunkType(other._chunkType),
		_sizeDescriptorByteCount(other._sizeDescriptorByteCount),
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
#ifdef _DEBUG
		output.flush();
#endif

		if (_chunkType == Type::Chunk)
		{
			stream::writeBigEndian<uint32_t>(output, getContentSize());
		}
		else
		{
			stream::writeBigEndian<uint16_t>(output, getContentSize());
		}

#ifdef _DEBUG
		output.flush();
#endif
		// Write the direct contents of this chunk
		std::string str = stream.str();
		output.write(str.c_str(), str.length());

#ifdef _DEBUG
		output.flush();
#endif

		// Write all subchunks
		for (const Chunk::Ptr& chunk : subChunks)
		{
			chunk->writeToStream(output);

#ifdef _DEBUG
			output.flush();
#endif

			// Add the padding byte after the chunk
			if (chunk->getContentSize() % 2 == 1)
			{
				output.write("\0", 1);
			}

#ifdef _DEBUG
			output.flush();
#endif
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

// Writes and S0 datatype to the given stream
void writeString(std::ostream& stream, const std::string& str)
{
	// LWO2 requires the following "Names or other character strings 
	// are written as a series of ASCII character values followed by 
	// a zero (or null) byte. If the length of the string including the 
	// null terminating byte is odd, an extra null is added so that the 
	// data that follows will begin on an even byte boundary."
	std::size_t len = str.length();

	// An empty string is a null-terminating byte plus the extra null
	if (len == 0)
	{
		stream.write("\0\0", 2);
		return;
	}

	// Write the string including the null-terminator
	stream.write(str.c_str(), len + 1);

	// Handle the extra padding byte
	if ((len + 1) % 2 == 1)
	{
		stream.write("\0", 1);
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
			writeString(tags->stream, surface.materialName);
		}
	}
	else
	{
		writeString(tags->stream, "");
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

	writeString(layr->stream, ""); // name[S0]
	// no parent index

	// Create the chunks for PNTS, POLS, PTAG, VMAP
	Chunk::Ptr pnts = fileChunk.addChunk("PNTS", Chunk::Type::Chunk);
	Chunk::Ptr bbox = fileChunk.addChunk("BBOX", Chunk::Type::Chunk);
	Chunk::Ptr pols = fileChunk.addChunk("POLS", Chunk::Type::Chunk);
	Chunk::Ptr ptag = fileChunk.addChunk("PTAG", Chunk::Type::Chunk);
	Chunk::Ptr vmap = fileChunk.addChunk("VMAP", Chunk::Type::Chunk);

	// We only ever export FACE polygons
	pols->stream.write("FACE", 4);
	ptag->stream.write("SURF", 4); // we tag the surfaces

	// VMAP { type[ID4], dimension[U2], name[S0], ...) }
	vmap->stream.write("TXUV", 4);		// "TXUV"
	stream::writeBigEndian<uint16_t>(vmap->stream, 2); // dimension
	
	std::string uvmapName = "UVMap";
	writeString(vmap->stream, uvmapName);

	std::size_t vertexIdxStart = 0;
	std::size_t polyNum = 0; // poly index is used across all surfaces

	AABB bounds;

	// Write all surface data
	for (std::size_t surfNum = 0; surfNum < _surfaces.size(); ++surfNum)
	{
		Surface& surface = _surfaces[surfNum];

		for (std::size_t v = 0; v < surface.vertices.size(); ++v)
		{
			const ArbitraryMeshVertex& vertex = surface.vertices[v];
			std::size_t vertNum = vertexIdxStart + v;

			// "The LightWave coordinate system is left-handed, with +X to the right or east, +Y upward, and +Z forward or north."
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.x()));
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.z()));
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.y()));

			// Write the UV map data (invert the T axis)
			writeVariableIndex(vmap->stream, vertNum);
			stream::writeBigEndian<float>(vmap->stream, static_cast<float>(vertex.texcoord.x()));
			stream::writeBigEndian<float>(vmap->stream, 1.0f - static_cast<float>(vertex.texcoord.y()));

			// Accumulate the BBOX
			bounds.includePoint(vertex.vertex);
		}

		int16_t numVerts = 3; // we export triangles

		// LWO2 sez: "When writing POLS, the vertex list for each polygon should begin 
		// at a convex vertex and proceed clockwise as seen from the visible side of the polygon"
		// DarkRadiant uses CCW windings, so reverse the index ordering

		for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3)
		{
			stream::writeBigEndian<uint16_t>(pols->stream, numVerts); // [U2]

			// The three vertices defining this polygon (reverse indices to produce LWO2 windings)
			writeVariableIndex(pols->stream, vertexIdxStart + surface.indices[i+2]); // [VX]
			writeVariableIndex(pols->stream, vertexIdxStart + surface.indices[i+1]); // [VX]
			writeVariableIndex(pols->stream, vertexIdxStart + surface.indices[i+0]); // [VX]

			// The surface mapping in the PTAG
			writeVariableIndex(ptag->stream, polyNum); // [VX]
			stream::writeBigEndian<uint16_t>(ptag->stream, static_cast<uint16_t>(surfNum)); // [U2]

			++polyNum;
		}

		// Write the SURF chunk for the surface
		Chunk::Ptr surf = fileChunk.addChunk("SURF", Chunk::Type::Chunk);

		writeString(surf->stream, surface.materialName);
		writeString(surf->stream, ""); // empty parent name

		// Define the base surface colour as <1.0, 1.0, 1.0>
		Chunk::Ptr colr = surf->addChunk("COLR", Chunk::Type::SubChunk);
		
		stream::writeBigEndian<float>(colr->stream, 1.0f);
		stream::writeBigEndian<float>(colr->stream, 1.0f);
		stream::writeBigEndian<float>(colr->stream, 1.0f);
		writeVariableIndex(colr->stream, 0);

		// Define the BLOK subchunk
		Chunk::Ptr blok = surf->addChunk("BLOK", Chunk::Type::SubChunk);

		// Add the IMAP subchunk
		Chunk::Ptr imap = blok->addChunk("IMAP", Chunk::Type::SubChunk);
		{
			// Use the same name as the surface as ordinal string
			writeString(imap->stream, surface.materialName);

			Chunk::Ptr imapChan = imap->addChunk("CHAN", Chunk::Type::SubChunk);
			imapChan->stream.write("COLR", 4);

			Chunk::Ptr imapEnab = imap->addChunk("ENAB", Chunk::Type::SubChunk);
			stream::writeBigEndian<uint16_t>(imapEnab->stream, 1);
		}

		// TMAP
		Chunk::Ptr blokTmap = blok->addChunk("TMAP", Chunk::Type::SubChunk);
		{
			Chunk::Ptr tmapSize = blokTmap->addChunk("SIZE", Chunk::Type::SubChunk);
			stream::writeBigEndian<float>(tmapSize->stream, 1.0f);
			stream::writeBigEndian<float>(tmapSize->stream, 1.0f);
			stream::writeBigEndian<float>(tmapSize->stream, 1.0f);
			writeVariableIndex(tmapSize->stream, 0);
		}

		// PROJ
		Chunk::Ptr blokProj = blok->addChunk("PROJ", Chunk::Type::SubChunk);
		stream::writeBigEndian<uint16_t>(blokProj->stream, 5); // UV-mapped projection

		// AXIS
		Chunk::Ptr blokAxis = blok->addChunk("AXIS", Chunk::Type::SubChunk);
		stream::writeBigEndian<uint16_t>(blokAxis->stream, 2); // Z axis

		// VMAP 
		Chunk::Ptr blokVmap = blok->addChunk("VMAP", Chunk::Type::SubChunk);
		writeString(blokVmap->stream, uvmapName);

		// Reposition the vertex index
		vertexIdxStart += surface.vertices.size();
	}

	// Write the bounds now that we know all the points
	Vector3 min = bounds.origin - bounds.extents;
	Vector3 max = bounds.origin + bounds.extents;

	stream::writeBigEndian<float>(bbox->stream, static_cast<float>(min.x()));
	stream::writeBigEndian<float>(bbox->stream, static_cast<float>(min.y()));
	stream::writeBigEndian<float>(bbox->stream, static_cast<float>(min.z()));

	stream::writeBigEndian<float>(bbox->stream, static_cast<float>(max.x()));
	stream::writeBigEndian<float>(bbox->stream, static_cast<float>(max.y()));
	stream::writeBigEndian<float>(bbox->stream, static_cast<float>(max.z()));

	fileChunk.writeToStream(stream);
}

}
