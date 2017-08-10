#include "Export.h"

#include <stdexcept>
#include "itextstream.h"
#include "model/ModelExporter.h"
#include "Traverse.h"
#include "os/path.h"

namespace map
{

namespace algorithm
{

void exportSelectedAsModel(const ModelExportOptions& options)
{
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
	std::string outputFile = os::getFilename(options.outputFilename);
	std::string outputPath = os::getContainingDir(options.outputFilename);

	rMessage() << "Exporting selection to file " << outputPath << outputFile << std::endl;

	model::ModelExporter::ExportToPath(expFormat, outputPath, outputFile);
}

void exportSelectedAsModelCmd(const cmd::ArgumentList& args)
{
	if (args.size() < 2 || args.size() > 4)
	{
		rMessage() << "Usage: ExportSelectedAsModel <Path> <ExportFormat> [<CenterObjects>] [<SkipCaulk>]" << std::endl;
		rMessage() << "   pass [<CenterObjects>] as 1 to center objects at 0,0,0 origin" << std::endl;
		rMessage() << "   pass [<SkipCaulk>] as 1 to skip caulked surfaces" << std::endl;
		return;
	}

	ModelExportOptions options;

	options.outputFilename = args[0].getString();
	options.outputFormat = args[1].getString();
	options.skipCaulk = false;
	options.centerObjects = false;

	if (args.size() >= 3)
	{
		options.centerObjects = (args[2].getInt() != 0);
	}

	if (args.size() >= 4)
	{
		options.skipCaulk = (args[3].getInt() != 0);
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
