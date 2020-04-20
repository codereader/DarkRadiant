#include "ScaledModelExporter.h"

#include <map>
#include <fstream>
#include "i18n.h"
#include "iundo.h"
#include "itextstream.h"
#include "igame.h"
#include "ientity.h"
#include "iscenegraph.h"
#include "os/fs.h"
#include "os/path.h"
#include "registry/registry.h"
#include <fmt/format.h>
#include "string/case_conv.h"
#include <regex>

#include "ModelExporter.h"

namespace map
{

void ScaledModelExporter::initialise()
{
	_mapEventConn = GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &ScaledModelExporter::onMapEvent)
	);
}

void ScaledModelExporter::shutdown()
{
	_mapEventConn.disconnect();
}

void ScaledModelExporter::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapSaving)
	{
		saveScaledModels();
	}
}

void ScaledModelExporter::saveScaledModels()
{
	std::map<scene::INodePtr, model::ModelNodePtr> nodesToProcess;

	// Find any models with modified scale
	GlobalSceneGraph().foreachNode([&](const scene::INodePtr& node)
	{
		if (Node_isEntity(node))
		{
			// Find any model nodes below that one
			node->foreachNode([&](const scene::INodePtr& child)
			{
				model::ModelNodePtr candidate = Node_getModel(child);

				if (candidate && candidate->hasModifiedScale())
				{
					nodesToProcess.insert(std::make_pair(node, candidate));
				}

				return true;
			});
		}

		return true;
	});

	// Do we have any models with modified scale?
	if (!nodesToProcess.empty())
	{
		UndoableCommand scaleModels("saveScaledModels");

		for (auto& pair : nodesToProcess)
		{
			saveScaledModel(pair.first, pair.second);
		}
	}
}

void ScaledModelExporter::saveScaledModel(const scene::INodePtr& entityNode, const model::ModelNodePtr& modelNode)
{
	// Request the default format from the preferences
	std::string outputExtension = registry::getValue<std::string>(RKEY_DEFAULT_MODEL_EXPORT_FORMAT);
	string::to_lower(outputExtension);

	rMessage() << "Model format used for export: " << outputExtension << 
		" (this can be changed in the preferences)" << std::endl;

	// Save the scaled model in the configured format
	model::IModelExporterPtr exporter = GlobalModelFormatManager().getExporter(outputExtension);

	if (!exporter)
	{
		rError() << "Cannot save out scaled models, no exporter found." << std::endl;
		return;
	}

	// Push the geometry into the exporter
	model::IModel& model = modelNode->getIModel();

	for (int s = 0; s < model.getSurfaceCount(); ++s)
	{
		const model::IModelSurface& surface = model.getSurface(s);

		exporter->addSurface(surface, Matrix4::getIdentity());
	}

	// Get the current model file name
	Entity* entity = Node_getEntity(entityNode);

	fs::path targetPath = getWritableGamePath();

	fs::path modelPath = "models/map_specific/scaled";

	// Ensure the output path exists
	targetPath /= modelPath;

	fs::create_directories(targetPath);

	fs::path modelKeyValue = entity->getKeyValue("model");

	rMessage() << "Exporting scaled model for entity " << entity->getKeyValue("name") << 
		": " << modelKeyValue.string() << std::endl;

	// Generate a new model name, à la "haystack_scaled3.ase"
	std::string modelFilename = generateUniqueModelFilename(targetPath, modelKeyValue, outputExtension);

	// assemble the new model spawnarg
	modelPath /= modelFilename;

	// Export to temporary file and rename afterwards
	try
	{
		model::ModelExporter::ExportToPath(exporter, targetPath.string(), modelFilename);

		std::string newModelKey = os::standardPath(modelPath.string());
		entity->setKeyValue("model", newModelKey);

		rMessage() << "Done exporting scaled model, new model key is " << newModelKey << std::endl;
	}
	catch (std::runtime_error& ex)
	{
		rError() << "Failed to export scaled model: " << ex.what() << std::endl;
	}
}

std::string ScaledModelExporter::generateUniqueModelFilename(
	const fs::path& outputPath, const fs::path& modelPath, const std::string& outputExtension)
{
	std::string modelFilename = modelPath.filename().string();

	// Remove any previously existing "_scaledN" suffix
	std::regex expr("_scaled\\d+\\.");
	modelFilename = std::regex_replace(modelFilename, expr, ".");

	std::string filenameNoExt = os::replaceExtension(modelFilename, "");

	int i = 0;

	while (++i < INT_MAX)
	{
		std::string generatedFilename = fmt::format("{0}_scaled{1:d}.{2}", filenameNoExt, i, outputExtension);
		fs::path targetFile = outputPath / generatedFilename;

		if (!fs::exists(targetFile))
		{
			return generatedFilename; // break the loop
		}
	}
	
	throw new std::runtime_error("Could not generate a unique model filename.");
}

fs::path ScaledModelExporter::getWritableGamePath()
{
	fs::path targetPath = GlobalGameManager().getModPath();

	if (targetPath.empty())
	{
		targetPath = GlobalGameManager().getUserEnginePath();

		rMessage() << "No mod base path found, falling back to user engine path to save model file: " <<
			targetPath.string() << std::endl;
	}

	return targetPath;
}

}
