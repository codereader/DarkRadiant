#include "Export.h"

#include <stdexcept>
#include "iundo.h"
#include "itextstream.h"

#include "os/path.h"

#include "selection/algorithm/Entity.h"
#include "selection/algorithm/General.h"
#include "model/ModelExporter.h"
#include "Traverse.h"

namespace map
{

namespace algorithm
{

void exportSelectedAsModel(const ModelExportOptions& options)
{
	if (!path_is_absolute(options.outputFilename.c_str()))
	{
		throw std::runtime_error("Output path must be absolute.");
	}

	std::string outputFormat = options.outputFormat;

	// Request the default format from the preferences
	if (outputFormat.empty())
	{
		outputFormat = registry::getValue<std::string>(RKEY_DEFAULT_MODEL_EXPORT_FORMAT);
	}

	boost::algorithm::to_lower(outputFormat);

	rMessage() << "Model format used for export: " << outputFormat << std::endl;

	// Get the output format
	model::IModelExporterPtr expFormat = GlobalModelFormatManager().getExporter(outputFormat);

	// Instantiate a ModelExporter to do the footwork
	model::ModelExporter exporter(expFormat);

	// Collect exportables
	// Call the traverseSelected function to hit the exporter with each node
	traverseSelected(GlobalSceneGraph().root(), exporter);

	exporter.setCenterObjects(options.centerObjects);
	exporter.setSkipCaulkMaterial(options.skipCaulk);

	exporter.processNodes();

	// Extract the output filename
	std::string absOutputPath = os::standardPath(options.outputFilename);

	std::string outputFile = os::getFilename(absOutputPath);
	std::string outputPath = os::getDirectory(absOutputPath);

	// Construct a mod-relative path from the absolute one
	std::string rootPath = GlobalFileSystem().findRoot(absOutputPath);

	// Check the pre-requisites before exporting
	if (options.replaceSelectionWithModel && rootPath.empty())
	{
		throw std::runtime_error(_("To replace the selection with the exported model\nthe output path must be located within the mod/project."));
	}

	rMessage() << "Exporting selection to file " << outputPath << outputFile << std::endl;

	model::ModelExporter::ExportToPath(expFormat, outputPath, outputFile);

	if (!options.replaceSelectionWithModel)
	{
		return; // done here
	}

	std::string relativeModelPath = os::getRelativePath(absOutputPath, rootPath);

	UndoableCommand command("replaceModel");

	// Remove the selection
	selection::algorithm::deleteSelection();

	// Create a func_static in its place
	try 
	{
		// Place the model in the world origin, unless we set "center objects" to true
		Vector3 modelPos(0, 0, 0);

		if (options.centerObjects)
		{
			modelPos = -exporter.getCenterTransform().t().getVector3();
		}

		scene::INodePtr modelNode = selection::algorithm::createEntityFromSelection("func_static", modelPos);
		
		Node_getEntity(modelNode)->setKeyValue("model", relativeModelPath);

		// It's possible that the export overwrote a model we're already using in this map, refresh it
		GlobalCommandSystem().executeCommand("RefreshSelectedModels");
	}
	catch (selection::algorithm::EntityCreationException&)
	{
		throw std::runtime_error(_("Unable to create model, classname not found."));
	}
}

void exportSelectedAsModelCmd(const cmd::ArgumentList& args)
{
	if (args.size() < 2 || args.size() > 5)
	{
		rMessage() << "Usage: ExportSelectedAsModel <Path> <ExportFormat> [<CenterObjects>] [<SkipCaulk>] [<ReplaceSelectionWithModel>]" << std::endl;
		rMessage() << "   <Path> must be an absolute file system path" << std::endl;
		rMessage() << "   pass [<CenterObjects>] as 1 to center objects at 0,0,0 origin" << std::endl;
		rMessage() << "   pass [<SkipCaulk>] as 1 to skip caulked surfaces" << std::endl;
		rMessage() << "   pass [<ReplaceSelectionWithModel>] as 1 to delete the selection and put the exported model in its place" << std::endl;
		return;
	}

	ModelExportOptions options;

	options.outputFilename = args[0].getString();
	options.outputFormat = args[1].getString();
	options.skipCaulk = false;
	options.centerObjects = false;
	options.replaceSelectionWithModel = false;

	if (args.size() >= 3)
	{
		options.centerObjects = (args[2].getInt() != 0);
	}

	if (args.size() >= 4)
	{
		options.skipCaulk = (args[3].getInt() != 0);
	}

	if (args.size() >= 4)
	{
		options.replaceSelectionWithModel = (args[4].getInt() != 0);
	}

	try
	{
		exportSelectedAsModel(options);
	}
	catch (std::runtime_error& ex)
	{
		rError() << "Failed to export model: " << ex.what() << std::endl;
	}
}

}

}
