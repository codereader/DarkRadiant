#include "ScaledModelExporter.h"

#include <fstream>
#include "i18n.h"
#include "itextstream.h"
#include "igame.h"
#include "ientity.h"
#include "iscenegraph.h"
#include "os/fs.h"
#include "os/path.h"
#include <boost/format.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <regex>

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
	// Find any models with modified scale
	GlobalSceneGraph().foreachNode([&](const scene::INodePtr& node)
	{
		if (Node_isEntity(node))
		{
			// Find any model nodes below that one
			model::ModelNodePtr childModel;

			node->foreachNode([&](const scene::INodePtr& child)
			{
				model::ModelNodePtr candidate = Node_getModel(child);

				if (candidate && candidate->hasModifiedScale())
				{
					childModel = candidate;
				}

				return true;
			});

			// Do we have a model with modified scale?
			if (childModel)
			{
				saveScaledModel(node, childModel);
			}
		}

		return true;
	});
}

void ScaledModelExporter::saveScaledModel(const scene::INodePtr& entityNode, const model::ModelNodePtr& modelNode)
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

	// Push the geometry into the exporter
	model::IModel& model = modelNode->getIModel();

	for (int s = 0; s < model.getSurfaceCount(); ++s)
	{
		const model::IModelSurface& surface = model.getSurface(s);

		exporter->addSurface(surface);
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
	exportModel(exporter, targetPath, modelFilename);

	std::string newModelKey = os::standardPath(modelPath.string());
	entity->setKeyValue("model", newModelKey);

	rMessage() << "Done exporting scaled model, new model key is " << newModelKey << std::endl;
}

void ScaledModelExporter::exportModel(const model::IModelExporterPtr& exporter,
	const fs::path& modelOutputPath, const std::string& modelFilename)
{
	fs::path targetPath = modelOutputPath;

	// Open a temporary file (leading underscore)
	fs::path tempFile = targetPath / ("_" + modelFilename);

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
	targetPath /= modelFilename;

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
		std::string generatedFilename = (boost::format("%s_scaled%d.%s") % filenameNoExt % i % outputExtension).str();
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
