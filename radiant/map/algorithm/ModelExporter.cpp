#include "ModelExporter.h"

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
		IBrush* brush = Node_getIBrush(node);

		if (brush == nullptr) return false;

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

				poly.a = convertWindingVertex(winding[i+1]);
				poly.b = convertWindingVertex(winding[i]);
				poly.c = convertWindingVertex(winding[0]);

				polys.push_back(poly);
			}

			_exporter->addPolygons(materialName, polys, node->localToWorld());
		}
	}

	return true;
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
