#include "ModelExporter.h"

#include "itextstream.h"
#include "imodel.h"
#include "registry/registry.h"
#include <boost/algorithm/string/case_conv.hpp>

namespace map
{

ModelExporter::ModelExporter()
{
	// Request the default format from the preferences
	std::string outputExtension = registry::getValue<std::string>(RKEY_DEFAULT_MODEL_EXPORT_FORMAT);
	boost::algorithm::to_lower(outputExtension);

	rMessage() << "Model format used for export: " << outputExtension <<
		" (this can be changed in the preferences)" << std::endl;

	// Save the scaled model as ASE
	model::IModelExporterPtr exporter = GlobalModelFormatManager().getExporter(outputExtension);

	if (!exporter)
	{
		rError() << "Cannot save out scaled models, no exporter found." << std::endl;
		return;
	}
}

bool ModelExporter::pre(const scene::INodePtr& node)
{
	if (Node_isModel(node))
	{
		model::ModelNodePtr modelNode = Node_getModel(node);

		// Push the geometry into the exporter
		model::IModel& model = modelNode->getIModel();

		for (int s = 0; s < model.getSurfaceCount(); ++s)
		{
			const model::IModelSurface& surface = model.getSurface(s);

			// exporter->addSurface(surface);
		}
	}

	return true;
}

void ModelExporter::post(const scene::INodePtr& node) 
{

}

}
