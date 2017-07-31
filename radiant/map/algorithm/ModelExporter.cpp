#include "ModelExporter.h"

#include "ibrush.h"
#include "ipatch.h"
#include "itextstream.h"
#include "imodel.h"

namespace map
{

namespace
{

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
	_exporter(exporter)
{
	if (!_exporter)
	{
		rError() << "Cannot save out scaled models, no exporter found." << std::endl;
		return;
	}
}

bool ModelExporter::pre(const scene::INodePtr& node)
{
	if (!_exporter) return false;

	if (Node_isModel(node))
	{
		model::ModelNodePtr modelNode = Node_getModel(node);

		// Push the geometry into the exporter
		model::IModel& model = modelNode->getIModel();

		for (int s = 0; s < model.getSurfaceCount(); ++s)
		{
			const model::IModelSurface& surface = model.getSurface(s);

			_exporter->addSurface(surface, node->localToWorld());
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

	return true;
}

void ModelExporter::processPatch(const scene::INodePtr& node)
{
	IPatch* patch = Node_getIPatch(node);

	if (patch == nullptr) return;

	const std::string& materialName = patch->getShader();
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

	_exporter->addPolygons(materialName, polys, node->localToWorld());
}

void ModelExporter::processBrush(const scene::INodePtr& node)
{
	IBrush* brush = Node_getIBrush(node);

	if (brush == nullptr) return;

	for (std::size_t b = 0; b < brush->getNumFaces(); ++b)
	{
		const IFace& face = brush->getFace(b);

		const std::string& materialName = face.getShader();
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

		_exporter->addPolygons(materialName, polys, node->localToWorld());
	}
}

void ModelExporter::post(const scene::INodePtr& node) 
{

}

void ModelExporter::writeToStream(std::ostream& stream)
{
	if (!_exporter) return;

	_exporter->exportToStream(stream);
}

}
