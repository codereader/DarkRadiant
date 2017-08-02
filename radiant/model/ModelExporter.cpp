#include "ModelExporter.h"

#include "i18n.h"
#include "ibrush.h"
#include "iclipper.h" // for caulk shader registry key
#include "ipatch.h"
#include "itextstream.h"
#include "imodel.h"
#include "os/fs.h"
#include "registry/registry.h"
#include <stdexcept>
#include <fstream>

namespace model
{

namespace
{

// Adapter methods to convert brush vertices to ArbitraryMeshVertex type
ArbitraryMeshVertex convertWindingVertex(const WindingVertex& in)
{
	ArbitraryMeshVertex out;

	out.vertex = in.vertex;
	out.normal = in.normal;
	out.texcoord = in.texcoord;
	out.bitangent = in.bitangent;
	out.tangent = in.tangent;
	out.colour.set(1.0, 1.0, 1.0);

	return out;
}

// Adapter methods to convert patch vertices to ArbitraryMeshVertex type
ArbitraryMeshVertex convertPatchVertex(const VertexNT& in)
{
	ArbitraryMeshVertex out;

	out.vertex = in.vertex;
	out.normal = in.normal;
	out.texcoord = in.texcoord;
	out.colour.set(1.0, 1.0, 1.0);

	return out;
}

}

ModelExporter::ModelExporter(const model::IModelExporterPtr& exporter) :
	_exporter(exporter),
	_skipCaulk(false),
	_caulkMaterial(registry::getValue<std::string>(RKEY_CLIPPER_CAULK_SHADER)),
	_centerObjects(false),
	_centerTransform(Matrix4::getIdentity())
{
	if (!_exporter)
	{
		rError() << "Cannot save out scaled models, no exporter found." << std::endl;
		return;
	}
}

void ModelExporter::setSkipCaulkMaterial(bool skipCaulk)
{
	_skipCaulk = skipCaulk;
}

void ModelExporter::setCenterObjects(bool centerObjects)
{
	_centerObjects = centerObjects;
}

bool ModelExporter::pre(const scene::INodePtr& node)
{
	if (!_exporter) return false;

	_nodes.push_back(node);

	return true;
}

void ModelExporter::processNodes()
{
	AABB bounds = calculateModelBounds();

	if (_centerObjects)
	{
		_centerTransform = Matrix4::getTranslation(-bounds.origin);
	}

	for (const scene::INodePtr& node : _nodes)
	{
		if (Node_isModel(node))
		{
			model::ModelNodePtr modelNode = Node_getModel(node);

			// Push the geometry into the exporter
			model::IModel& model = modelNode->getIModel();

			for (int s = 0; s < model.getSurfaceCount(); ++s)
			{
				const model::IModelSurface& surface = model.getSurface(s);

				if (isExportableMaterial(surface.getDefaultMaterial()))
				{
					_exporter->addSurface(surface, node->localToWorld().getMultipliedBy(_centerTransform));
				}
			}
		}
		else if (Node_isBrush(node))
		{
			processBrush(node);
		}
		else if (Node_isPatch(node))
		{
			processPatch(node);
		}
	}
}

AABB ModelExporter::calculateModelBounds()
{
	AABB bounds;

	for (const scene::INodePtr& node : _nodes)
	{
		bounds.includeAABB(node->worldAABB());
	}

	return bounds;
}

void ModelExporter::processPatch(const scene::INodePtr& node)
{
	IPatch* patch = Node_getIPatch(node);

	if (patch == nullptr) return;

	const std::string& materialName = patch->getShader();

	if (!isExportableMaterial(materialName)) return;

	PatchMesh mesh = patch->getTesselatedPatchMesh();

	std::vector<model::ModelPolygon> polys;

	for (std::size_t h = 0; h < mesh.height - 1; ++h)
	{
		for (std::size_t w = 0; w < mesh.width - 1; ++w)
		{
			model::ModelPolygon poly;

			poly.a = convertPatchVertex(mesh.vertices[w + (h*mesh.width)]);
			poly.b = convertPatchVertex(mesh.vertices[w + 1 + (h*mesh.width)]);
			poly.c = convertPatchVertex(mesh.vertices[w + mesh.width + (h*mesh.width)]);

			polys.push_back(poly);
				
			poly.a = convertPatchVertex(mesh.vertices[w + 1 + (h*mesh.width)]);
			poly.b = convertPatchVertex(mesh.vertices[w + 1 + mesh.width + (h*mesh.width)]);
			poly.c = convertPatchVertex(mesh.vertices[w + mesh.width + (h*mesh.width)]);

			polys.push_back(poly);
		}
	}

	_exporter->addPolygons(materialName, polys, node->localToWorld().getMultipliedBy(_centerTransform));
}

void ModelExporter::processBrush(const scene::INodePtr& node)
{
	IBrush* brush = Node_getIBrush(node);

	if (brush == nullptr) return;

	for (std::size_t b = 0; b < brush->getNumFaces(); ++b)
	{
		const IFace& face = brush->getFace(b);

		const std::string& materialName = face.getShader();

		if (!isExportableMaterial(materialName)) continue;

		const IWinding& winding = face.getWinding();

		std::vector<model::ModelPolygon> polys;

		if (winding.size() < 3)
		{
			rWarning() << "Skipping face with less than 3 winding verts" << std::endl;
			continue;
		}

		// Create triangles for this winding 
		for (std::size_t i = 1; i < winding.size() - 1; ++i)
		{
			model::ModelPolygon poly;

			poly.a = convertWindingVertex(winding[i + 1]);
			poly.b = convertWindingVertex(winding[i]);
			poly.c = convertWindingVertex(winding[0]);

			polys.push_back(poly);
		}

		_exporter->addPolygons(materialName, polys, node->localToWorld().getMultipliedBy(_centerTransform));
	}
}

bool ModelExporter::isExportableMaterial(const std::string& materialName)
{
	return !_skipCaulk || materialName != _caulkMaterial;
}

void ModelExporter::ExportToPath(const model::IModelExporterPtr& exporter,
	const std::string& outputPath, const std::string& filename)
{
	fs::path targetPath = outputPath;

	// Open a temporary file (leading underscore)
	fs::path tempFile = targetPath / ("_" + filename);

	std::ofstream::openmode mode = std::ofstream::out;

	if (exporter->getFileFormat() == model::IModelExporter::Format::Binary)
	{
		mode |= std::ios::binary;
	}

	std::ofstream tempStream(tempFile.string().c_str(), mode);

	if (!tempStream.is_open())
	{
		throw std::runtime_error(
			(boost::format(_("Cannot open file for writing: %s")) % tempFile.string()).str());
	}

	exporter->exportToStream(tempStream);

	tempStream.close();

	// The full OS path to the output file
	targetPath /= filename;

	if (fs::exists(targetPath))
	{
		try
		{
			fs::remove(targetPath);
		}
		catch (fs::filesystem_error& e)
		{
			rError() << "Could not remove the file " << targetPath.string() << std::endl
				<< e.what() << std::endl;

			throw std::runtime_error(
				(boost::format(_("Could not remove the file: %s")) % tempFile.string()).str());
		}
	}

	try
	{
		fs::rename(tempFile, targetPath);
	}
	catch (fs::filesystem_error& e)
	{
		rError() << "Could not rename the temporary file " << tempFile.string() << std::endl
			<< e.what() << std::endl;

		throw std::runtime_error(
			(boost::format(_("Could not rename the temporary file: %s")) % tempFile.string()).str());
	}
}

}
