#include "Lwo2Exporter.h"

#include <vector>
#include "itextstream.h"
#include "imodelsurface.h"
#include "imap.h"
#include "math/AABB.h"
#include "stream/utils.h"

#include "Lwo2Chunk.h"

// Namespace extension containing some LWO-specific data export functions
namespace stream
{

// Write a Variable Index (VX) data type to the given stream
void writeVariableIndex(std::ostream& stream, std::size_t index)
{
	// LWO2 defines the variable index VX data type which is
	// 32 bit as soon as the index value is greater than 0xFF00, otherwise 16 bit
	if (index < 0xFF00)
	{
		writeBigEndian<uint16_t>(stream, static_cast<uint16_t>(index));
	}
	else
	{
		// According to the specs, for values greater than 0xFF00:
		// "the index is written as an unsigned four byte integer with bits 24-31 set"
		writeBigEndian<uint32_t>(stream, static_cast<uint32_t>(index) | 0xFF000000);
	}
}

// Writes an S0 datatype to the given stream
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

} // namespace stream

namespace model
{

IModelExporterPtr Lwo2Exporter::clone()
{
	return std::make_shared<Lwo2Exporter>();
}

const std::string& Lwo2Exporter::getDisplayName() const
{
	static std::string _extension("Lightwave Object File");
	return _extension;
}

const std::string& Lwo2Exporter::getExtension() const
{
	static std::string _extension("LWO");
	return _extension;
}

void Lwo2Exporter::exportToPath(const std::string& outputPath, const std::string& filename)
{
    // Open the stream to the output file
    stream::ExportStream output(outputPath, filename, stream::ExportStream::Mode::Binary);

    exportToStream(output.getStream());

    output.close();
}

void Lwo2Exporter::exportToStream(std::ostream& stream)
{
	// The encompassing FORM chunk
	Lwo2Chunk fileChunk("FORM", Lwo2Chunk::Type::Chunk);

	// The data of the FORM file contains just the LWO2 id and the collection of chunks
	fileChunk.stream.write("LWO2", 4);

	// Assemble the list of regular Chunks, these all use 4 bytes for size info

	// TAGS
	Lwo2Chunk::Ptr tags = fileChunk.addChunk("TAGS");
	
	// Export all material names as tags
	if (!_surfaces.empty())
	{
		for (const Surfaces::value_type& pair : _surfaces)
		{
			stream::writeString(tags->stream, pair.second.materialName);
		}
	}
	else
	{
		stream::writeString(tags->stream, "");
	}

	// Create a single layer for the geometry
	Lwo2Chunk::Ptr layr = fileChunk.addChunk("LAYR");

	// LAYR{ number[U2], flags[U2], pivot[VEC12], name[S0], parent[U2] ? }

	stream::writeBigEndian<uint16_t>(layr->stream, 0); // number[U2]
	stream::writeBigEndian<uint16_t>(layr->stream, 0); // flags[U2]

	// pivot[VEC12]
	stream::writeBigEndian<float>(layr->stream, 0);
	stream::writeBigEndian<float>(layr->stream, 0);
	stream::writeBigEndian<float>(layr->stream, 0);

	stream::writeString(layr->stream, ""); // name[S0]
	// no parent index

	// Create the chunks for PNTS, POLS, PTAG, VMAP
	Lwo2Chunk::Ptr pnts = fileChunk.addChunk("PNTS");
	Lwo2Chunk::Ptr bbox = fileChunk.addChunk("BBOX");
	Lwo2Chunk::Ptr pols = fileChunk.addChunk("POLS");
	Lwo2Chunk::Ptr ptag = fileChunk.addChunk("PTAG");
	Lwo2Chunk::Ptr vmap = fileChunk.addChunk("VMAP");
	Lwo2Chunk::Ptr colourVmap = fileChunk.addChunk("VMAP");

	// We only ever export FACE polygons
	pols->stream.write("FACE", 4);
	ptag->stream.write("SURF", 4); // we tag the surfaces

	// Texture UV Coordinates go into one VMAP
	// VMAP { type[ID4], dimension[U2], name[S0], ...) }
	vmap->stream.write("TXUV", 4);		// "TXUV"
	stream::writeBigEndian<uint16_t>(vmap->stream, 2); // dimension (2 vector components)
	
	std::string uvmapName = "UVMap";
	stream::writeString(vmap->stream, uvmapName);

	// Vertex Colours go into another VMAP
	// VMAP { type[ID4], dimension[U2], name[S0], ...) }
	colourVmap->stream.write("RGBA", 4); // type [ID4] == "RGBA"
	stream::writeBigEndian<uint16_t>(colourVmap->stream, 4); // dimension (4 colour components)

	std::string vertColourMapName = "VertexColourMap";
	stream::writeString(colourVmap->stream, vertColourMapName); // map name [S0]

	std::size_t vertexIdxStart = 0;
	std::size_t polyNum = 0; // poly index is used across all surfaces

	AABB bounds;

	// Write all surface data
	std::size_t surfNum = 0;
	for (Surfaces::value_type& pair : _surfaces)
	{
		Surface& surface = pair.second;

		for (std::size_t v = 0; v < surface.vertices.size(); ++v)
		{
			const MeshVertex& vertex = surface.vertices[v];
			std::size_t vertNum = vertexIdxStart + v;

			// "The LightWave coordinate system is left-handed, with +X to the right or east, +Y upward, and +Z forward or north."
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.x()));
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.z()));
			stream::writeBigEndian<float>(pnts->stream, static_cast<float>(vertex.vertex.y()));

			// Write the UV map data (invert the T axis)
			stream::writeVariableIndex(vmap->stream, vertNum);
			stream::writeBigEndian<float>(vmap->stream, static_cast<float>(vertex.texcoord.x()));
			stream::writeBigEndian<float>(vmap->stream, 1.0f - static_cast<float>(vertex.texcoord.y()));

			// Write the vertex colour data
			stream::writeVariableIndex(colourVmap->stream, vertNum);
			stream::writeBigEndian<float>(colourVmap->stream, static_cast<float>(vertex.colour.x()));
			stream::writeBigEndian<float>(colourVmap->stream, static_cast<float>(vertex.colour.y()));
			stream::writeBigEndian<float>(colourVmap->stream, static_cast<float>(vertex.colour.z()));
			stream::writeBigEndian<float>(colourVmap->stream, static_cast<float>(vertex.colour.w()));

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
			stream::writeVariableIndex(pols->stream, vertexIdxStart + surface.indices[i+2]); // [VX]
			stream::writeVariableIndex(pols->stream, vertexIdxStart + surface.indices[i+1]); // [VX]
			stream::writeVariableIndex(pols->stream, vertexIdxStart + surface.indices[i+0]); // [VX]

			// The surface mapping in the PTAG
			stream::writeVariableIndex(ptag->stream, polyNum); // [VX]
			stream::writeBigEndian<uint16_t>(ptag->stream, static_cast<uint16_t>(surfNum)); // [U2]

			++polyNum;
		}

		// Write the SURF chunk for the surface
		Lwo2Chunk::Ptr surf = fileChunk.addChunk("SURF");

		stream::writeString(surf->stream, surface.materialName);
		stream::writeString(surf->stream, ""); // empty parent name

		// Define the base surface colour as <1.0, 1.0, 1.0>
		Lwo2Chunk::Ptr colr = surf->addSubChunk("COLR");
		
		stream::writeBigEndian<float>(colr->stream, 1.0f);
		stream::writeBigEndian<float>(colr->stream, 1.0f);
		stream::writeBigEndian<float>(colr->stream, 1.0f);
		stream::writeVariableIndex(colr->stream, 0);

		// Reference the name of the vertex colour map
		Lwo2Chunk::Ptr vcol = surf->addSubChunk("VCOL");
		stream::writeBigEndian<float>(vcol->stream, 1.0f); // intensity [F4]
		stream::writeVariableIndex(vcol->stream, 0); // [VX]
		vcol->stream.write("RGBA", 4); // vmap-type [ID4]
		stream::writeString(vcol->stream, vertColourMapName); // name [S0]

		// Smoothing angle
		Lwo2Chunk::Ptr sman = surf->addSubChunk("SMAN");
		stream::writeBigEndian<float>(sman->stream, static_cast<float>(degrees_to_radians(95.0f))); // 95 degrees smoothing angle

		// Define the BLOK subchunk
		Lwo2Chunk::Ptr blok = surf->addSubChunk("BLOK");

		// Add the IMAP subchunk
		Lwo2Chunk::Ptr imap = blok->addSubChunk("IMAP");
		{
			// Use the same name as the surface as ordinal string
			stream::writeString(imap->stream, surface.materialName);

			Lwo2Chunk::Ptr imapChan = imap->addSubChunk("CHAN");
			imapChan->stream.write("COLR", 4);

			Lwo2Chunk::Ptr imapEnab = imap->addSubChunk("ENAB");
			stream::writeBigEndian<uint16_t>(imapEnab->stream, 1);
		}

		// TMAP
		Lwo2Chunk::Ptr blokTmap = blok->addSubChunk("TMAP");
		{
			Lwo2Chunk::Ptr tmapSize = blokTmap->addSubChunk("SIZE");
			stream::writeBigEndian<float>(tmapSize->stream, 1.0f);
			stream::writeBigEndian<float>(tmapSize->stream, 1.0f);
			stream::writeBigEndian<float>(tmapSize->stream, 1.0f);
			stream::writeVariableIndex(tmapSize->stream, 0);
		}

		// PROJ
		Lwo2Chunk::Ptr blokProj = blok->addSubChunk("PROJ");
		stream::writeBigEndian<uint16_t>(blokProj->stream, 5); // UV-mapped projection

		// AXIS
		Lwo2Chunk::Ptr blokAxis = blok->addSubChunk("AXIS");
		stream::writeBigEndian<uint16_t>(blokAxis->stream, 2); // Z axis

		// VMAP 
		Lwo2Chunk::Ptr blokVmap = blok->addSubChunk("VMAP");
		stream::writeString(blokVmap->stream, uvmapName);

		// Reposition the vertex index
		vertexIdxStart += surface.vertices.size();

		++surfNum;
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
