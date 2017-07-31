#include "ModelExporter.h"

#include "itextstream.h"
#include "imodel.h"

namespace map
{

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

			_exporter->addSurface(surface);
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
