#include "AseExporter.h"

#include "itextstream.h"
#include "imodelsurface.h"
#include "imap.h"
#include "string/replace.h"
#include <fmt/format.h>

namespace model
{

IModelExporterPtr AseExporter::clone()
{
	return std::make_shared<AseExporter>();
}

const std::string& AseExporter::getDisplayName() const
{
	static std::string _extension("ASCII Scene Export");
	return _extension;
}

const std::string& AseExporter::getExtension() const
{
	static std::string _extension("ASE");
	return _extension;
}

void AseExporter::exportToPath(const std::string& outputPath, const std::string& filename)
{
    // Open the stream to the output file
    stream::ExportStream output(outputPath, filename, stream::ExportStream::Mode::Text);

    exportToStream(output.getStream());

    output.close();
}

void AseExporter::exportToStream(std::ostream& stream)
{
	// Header / scene block
	stream << "*3DSMAX_ASCIIEXPORT	200" << std::endl;
	stream << "*COMMENT \"DarkRadiant ASCII Scene Export(*.ase)\"" << std::endl;
	stream << "*SCENE {" << std::endl;
	stream << "\t*SCENE_FILENAME \"" << GlobalMapModule().getMapName() << "\"" << std::endl;
	stream << "\t*SCENE_FIRSTFRAME 0" << std::endl;
	stream << "\t*SCENE_LASTFRAME 100" << std::endl;
	stream << "\t*SCENE_FRAMESPEED 30" << std::endl;
	stream << "\t*SCENE_TICKSPERFRAME 160" << std::endl;
	stream << "\t*SCENE_BACKGROUND_STATIC 0.0000	0.0000	0.0000" << std::endl;
	stream << "\t*SCENE_AMBIENT_STATIC 0.0000	0.0000	0.0000" << std::endl;
	stream << "}" << std::endl;

	// Remove empty surfaces before exporting (#5104)
	for (auto it = _surfaces.begin(); it != _surfaces.end();)
	{
		if (it->second.vertices.empty())
		{
			_surfaces.erase(it++);
		}
		else
		{
			++it;
		}
	}

	// Materials
	stream << "*MATERIAL_LIST {" << std::endl;
	stream << "\t*MATERIAL_COUNT " << _surfaces.size() << std::endl;

	std::size_t m = 0;

	for (const Surfaces::value_type& pair : _surfaces)
	{
		std::string aseMaterial = pair.second.materialName;
		string::replace_all(aseMaterial, "/", "\\");

		stream << "\t*MATERIAL " << m << " {" << std::endl;
		stream << "\t\t*MATERIAL_NAME \"" << aseMaterial << "\"" << std::endl;
		stream << "\t\t*MATERIAL_CLASS \"Standard\"" << std::endl;
		stream << "\t\t*MATERIAL_AMBIENT 0.5882	0.5882	0.5882" << std::endl;
		stream << "\t\t*MATERIAL_DIFFUSE 0.5882	0.5882	0.5882" << std::endl;
		stream << "\t\t*MATERIAL_SPECULAR 0.9000	0.9000	0.9000" << std::endl;
		stream << "\t\t*MATERIAL_SHINE 0.1000" << std::endl;
		stream << "\t\t*MATERIAL_SHINESTRENGTH 0.0000" << std::endl;
		stream << "\t\t*MATERIAL_TRANSPARENCY 0.0000" << std::endl;
		stream << "\t\t*MATERIAL_WIRESIZE 1.0000" << std::endl;
		stream << "\t\t*MATERIAL_SHADING Blinn" << std::endl;
		stream << "\t\t*MATERIAL_XP_FALLOFF 0.0000" << std::endl;
		stream << "\t\t*MATERIAL_SELFILLUM 0.0000" << std::endl;
		stream << "\t\t*MATERIAL_FALLOFF In" << std::endl;
		stream << "\t\t*MATERIAL_XP_TYPE Filter" << std::endl;
		stream << "\t\t*MAP_DIFFUSE {" << std::endl;
		stream << "\t\t\t*MAP_NAME \"" << aseMaterial << "\"" << std::endl;
		stream << "\t\t\t*MAP_CLASS \"Bitmap\"" << std::endl;
		stream << "\t\t\t*MAP_SUBNO 1" << std::endl;
		stream << "\t\t\t*MAP_AMOUNT 1.0000" << std::endl;
		stream << "\t\t\t*BITMAP \"\\\\base\\" << aseMaterial << "\"" << std::endl;
		stream << "\t\t\t*MAP_TYPE Screen" << std::endl;
		stream << "\t\t\t*UVW_U_OFFSET 0.0000" << std::endl;
		stream << "\t\t\t*UVW_V_OFFSET 0.0000" << std::endl;
		stream << "\t\t\t*UVW_U_TILING 1.0000" << std::endl;
		stream << "\t\t\t*UVW_V_TILING 1.0000" << std::endl;
		stream << "\t\t\t*UVW_ANGLE 0.0000" << std::endl;
		stream << "\t\t\t*UVW_BLUR 1.0000" << std::endl;
		stream << "\t\t\t*UVW_BLUR_OFFSET 0.0000" << std::endl;
		stream << "\t\t\t*UVW_NOUSE_AMT 1.0000" << std::endl;
		stream << "\t\t\t*UVW_NOISE_SIZE 1.0000" << std::endl;
		stream << "\t\t\t*UVW_NOISE_LEVEL 1" << std::endl;
		stream << "\t\t\t*UVW_NOISE_PHASE 0.0000" << std::endl;
		stream << "\t\t\t*BITMAP_FILTER Pyramidal" << std::endl;
		stream << "\t\t}" << std::endl;
		stream << "\t}" << std::endl;

		++m;
	}

	stream << "}" << std::endl; // Material List End

	// Geom Objects
	m = 0;

	for (const Surfaces::value_type& pair : _surfaces)
	{
		const Surface& surface = pair.second;

		stream << "*GEOMOBJECT {" << std::endl;

		stream << "\t*NODE_NAME \"mesh" << m << "\"" << std::endl;
		stream << "\t*NODE_TM {" << std::endl;
		stream << "\t\t*NODE_NAME \"mesh" << m << "\"" << std::endl;
		stream << "\t\t*INHERIT_POS 0 0 0" << std::endl;
		stream << "\t\t*INHERIT_ROT 0 0 0" << std::endl;
		stream << "\t\t*INHERIT_SCL 0 0 0" << std::endl;
		stream << "\t\t*TM_ROW0 1.0000	0.0000	0.0000" << std::endl;
		stream << "\t\t*TM_ROW1 0.0000	1.0000	0.0000" << std::endl;
		stream << "\t\t*TM_ROW2 0.0000	0.0000	1.0000" << std::endl;
		stream << "\t\t*TM_ROW3 0.0000	0.0000	0.0000" << std::endl;
		stream << "\t\t*TM_POS 0.0000	0.0000	0.0000" << std::endl;
		stream << "\t\t*TM_ROTAXIS 0.0000	0.0000	0.0000" << std::endl;
		stream << "\t\t*TM_ROTANGLE 0.0000" << std::endl;
		stream << "\t\t*TM_SCALE 1.0000	1.0000	1.0000" << std::endl;
		stream << "\t\t*TM_SCALEAXIS 0.0000	0.0000	0.0000" << std::endl;
		stream << "\t\t*TM_SCALEAXISANG 0.0000" << std::endl;
		stream << "\t}" << std::endl;

		stream << "\t*MESH {" << std::endl;

		stream << "\t\t*TIMEVALUE 0" << std::endl;
		stream << "\t\t*MESH_NUMVERTEX " << surface.vertices.size() << std::endl;
		stream << "\t\t*MESH_NUMFACES " << (surface.indices.size() / 3) << std::endl;

		// Vertices
		stream << "\t\t*MESH_VERTEX_LIST {" << std::endl;

		for (std::size_t v = 0; v < surface.vertices.size(); ++v)
		{
			const Vertex3f& vert = surface.vertices[v].vertex;

			stream << "\t\t\t*MESH_VERTEX " << v << "\t" << vert.x() << "\t" << vert.y() << "\t" << vert.z() << std::endl;
		}

		stream << "\t\t}" << std::endl;

		// Faces
		stream << "\t\t*MESH_FACE_LIST {" << std::endl;

		for (std::size_t i = 0; i+2 < surface.indices.size(); i += 3)
		{
			std::size_t faceNum = i / 3;

			stream << fmt::format("\t\t\t*MESH_FACE {:3d}:  A: {:3d} B: {:3d} C: {:3d} AB:       0 BC:    0 CA:    0	 *MESH_SMOOTHING 1 	*MESH_MTLID {:3d}",
				faceNum, surface.indices[i], surface.indices[i + 1], surface.indices[i + 2], m) << std::endl;
		}

		stream << "\t\t}" << std::endl;

		stream << "\t\t*MESH_NUMTVERTEX " << surface.vertices.size() << std::endl;

		stream << "\t\t*MESH_TVERTLIST {" << std::endl;

		for (std::size_t v = 0; v < surface.vertices.size(); ++v)
		{
			const TexCoord2f& tex = surface.vertices[v].texcoord;
			
			// Invert the T coordinate
			stream << "\t\t\t*MESH_TVERT " << v << "\t" << tex.x() << "\t" << (-tex.y()) << "\t0.0000" << std::endl;
		}

		stream << "\t\t}" << std::endl;
		
		// TFaces
		stream << "\t\t*MESH_NUMTVFACES " << (surface.indices.size() / 3) << std::endl;
		stream << "\t\t*MESH_TFACELIST {" << std::endl;

		for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3)
		{
			std::size_t faceNum = i / 3;

			stream << fmt::format("\t\t\t*MESH_TFACE {:3d}\t{:3d}\t{:3d}\t{:3d}",
				faceNum, surface.indices[i], surface.indices[i + 1], surface.indices[i + 2]) << std::endl;
		}

		stream << "\t\t}" << std::endl;

		// CVerts
		stream << "\t\t*MESH_NUMCVERTEX " << surface.vertices.size() << std::endl;

		stream << "\t\t*MESH_CVERTLIST {" << std::endl;

		for (std::size_t v = 0; v < surface.vertices.size(); ++v)
		{
			const auto& vcol = surface.vertices[v].colour.getVector3();

			stream << "\t\t\t*MESH_VERTCOL " << v << "\t" << vcol.x() << "\t" << vcol.y() << "\t" << vcol.z() << std::endl;
		}

		stream << "\t\t}" << std::endl;

		// CFaces
		stream << "\t\t*MESH_NUMCVFACES " << (surface.indices.size() / 3) << std::endl;
		stream << "\t\t*MESH_CFACELIST {" << std::endl;

		for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3)
		{
			std::size_t faceNum = i / 3;

			stream << fmt::format("\t\t\t*MESH_CFACE {:3d}\t{:3d}\t{:3d}\t{:3d}",
				faceNum, surface.indices[i], surface.indices[i + 1], surface.indices[i + 2]) << std::endl;
		}

		stream << "\t\t}" << std::endl;
		
		stream << "\t\t*MESH_NORMALS { " << std::endl;

		for (std::size_t i = 0; i + 2 < surface.indices.size(); i += 3)
		{
			std::size_t faceNum = i / 3;

			const Normal3f& normal1 = surface.vertices[surface.indices[i]].normal;
			const Normal3f& normal2 = surface.vertices[surface.indices[i+1]].normal;
			const Normal3f& normal3 = surface.vertices[surface.indices[i+2]].normal;

			stream << "\t\t\t*MESH_FACENORMAL " << faceNum << "\t" << normal1.x() << "\t" << normal1.y() << "\t" << normal1.z() << std::endl;

			stream << "\t\t\t\t*MESH_VERTEXNORMAL " << surface.indices[i] << "\t" << normal1.x() << "\t" << normal1.y() << "\t" << normal1.z() << std::endl;
			stream << "\t\t\t\t*MESH_VERTEXNORMAL " << surface.indices[i+1] << "\t" << normal2.x() << "\t" << normal2.y() << "\t" << normal2.z() << std::endl;
			stream << "\t\t\t\t*MESH_VERTEXNORMAL " << surface.indices[i+2] << "\t" << normal3.x() << "\t" << normal3.y() << "\t" << normal3.z() << std::endl;
		}

		stream << "\t\t}" << std::endl;

		stream << "\t}" << std::endl;

		stream << "\t*PROP_MOTIONBLUR 0" << std::endl;
		stream << "\t*PROP_CASTSHADOW 1" << std::endl;
		stream << "\t*PROP_RECVSHADOW 1" << std::endl;
		stream << "\t*MATERIAL_REF " << m << std::endl;

		stream << "}" << std::endl;

		++m;
	}
}

}
