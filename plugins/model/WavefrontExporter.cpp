#include "WavefrontExporter.h"

#include "itextstream.h"
#include "imodelsurface.h"
#include "imap.h"

namespace model
{

WavefrontExporter::WavefrontExporter()
{}

IModelExporterPtr WavefrontExporter::clone()
{
	return std::make_shared<WavefrontExporter>();
}

const std::string& WavefrontExporter::getDisplayName() const
{
	static std::string _extension("Wavefront OBJ");
	return _extension;
}

const std::string& WavefrontExporter::getExtension() const
{
	static std::string _extension("OBJ");
	return _extension;
}

void WavefrontExporter::exportToStream(std::ostream& stream)
{
	// Count exported vertices. Exported indices are 1-based though.
	std::size_t vertexCount = 0;

	// Each surface is exported as group. 
	for (const Surfaces::value_type& pair : _surfaces)
	{
		const Surface& surface = pair.second;

		// Base index for vertices, added to the surface indices
		std::size_t vertBaseIndex = vertexCount;

		// Since we don't write .mtl files store at least the material into the group name
		stream << "g " << surface.materialName << std::endl;
		stream << std::endl;

		// Temporary buffers for vertices, texcoords and polys
		std::stringstream vertexBuf;
		std::stringstream texCoordBuf;
		std::stringstream polyBuf;

		for (const ArbitraryMeshVertex& meshVertex : surface.vertices)
		{
			// Write coordinates into the export buffers
			const Vector3& vert = meshVertex.vertex;
			const Vector2& uv = meshVertex.texcoord;

			vertexBuf << "v " << vert.x() << " " << vert.y() << " " << vert.z() << "\n";
			texCoordBuf << "vt " << uv.x() << " " << uv.y() << "\n";

			vertexCount++;
		}

		// Every three indices form a triangle. Indices are 1-based so add +1 to each index
		for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3)
		{ 
			std::size_t index1 = vertBaseIndex + static_cast<std::size_t>(surface.indices[i+0]) + 1;
			std::size_t index2 = vertBaseIndex + static_cast<std::size_t>(surface.indices[i+1]) + 1;
			std::size_t index3 = vertBaseIndex + static_cast<std::size_t>(surface.indices[i+2]) + 1;

			// f 1/1 3/3 2/2
			polyBuf << "f";
			polyBuf << " " << index1 << "/" << index1;
			polyBuf << " " << index2 << "/" << index2;
			polyBuf << " " << index3 << "/" << index3;
			polyBuf << "\n";
		}

		stream << vertexBuf.str() << std::endl;
		stream << texCoordBuf.str() << std::endl;
		stream << polyBuf.str() << std::endl;
	}
}

}
