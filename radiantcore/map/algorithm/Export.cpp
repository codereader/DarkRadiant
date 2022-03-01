#include "Export.h"

#include <stdexcept>
#include "i18n.h"
#include "ifilesystem.h"
#include "imodelcache.h"
#include "ientity.h"
#include "iundo.h"
#include "itextstream.h"

#include "os/path.h"

#include "selectionlib.h"
#include "selection/algorithm/Entity.h"
#include "selection/algorithm/General.h"
#include "string/convert.h"
#include "string/case_conv.h"
#include "scenelib.h"
#include "model/export/ModelExporter.h"
#include "registry/registry.h"
#include "scene/Traverse.h"
#include "command/ExecutionFailure.h"
#include "Models.h"

namespace map
{

namespace algorithm
{

// Returns the union set of layer IDs of the current selection
scene::LayerList getAllLayersOfSelection()
{
    scene::LayerList unionSet;

    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        const auto& layers = node->getLayers();
        unionSet.insert(layers.begin(), layers.end());
    });

    return unionSet;
}

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

	string::to_lower(outputFormat);

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
	exporter.setExportLightsAsObjects(options.exportLightsAsObjects);

	if (options.useEntityOrigin)
	{
		// Check if we have a single entity selected
		const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

		if (info.totalCount == 1 && info.entityCount == 1)
		{
			Entity* entity = Node_getEntity(GlobalSelectionSystem().ultimateSelected());

			if (entity != nullptr)
			{
				Vector3 entityOrigin = string::convert<Vector3>(entity->getKeyValue("origin"));
				exporter.setOrigin(entityOrigin);
			}
		}
		else
		{
			rWarning() << "Will ignore the UseEntityOrigin setting as we don't have a single entity selected." << std::endl;
		}
	}

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

    expFormat->exportToPath(outputPath, outputFile);

    std::string relativeModelPath = os::getRelativePath(absOutputPath, rootPath);

    if (options.replaceSelectionWithModel)
    {
        UndoableCommand command("replaceModel");

        // Remember the last selected entity to preserve spawnargs
        auto lastSelectedNode = GlobalSelectionSystem().ultimateSelected();
        auto lastSelectedEntity = Node_getEntity(lastSelectedNode);
        auto root = lastSelectedNode->getRootNode();

        // Remove the selection, but remember its layers first
        auto previousLayerSet = getAllLayersOfSelection();
        selection::algorithm::deleteSelection();

        // Create an entity of the same class in its place
        try
        {
            // Place the model in the world origin, unless we set "center objects" to true
            Vector3 modelPos(0, 0, 0);

            if (options.centerObjects)
            {
                modelPos = -exporter.getCenterTransform().translation();
            }

            auto className = lastSelectedEntity ? lastSelectedEntity->getKeyValue("classname") : "func_static";
            auto eclass = GlobalEntityClassManager().findOrInsert(className, false);
            auto modelNode = GlobalEntityModule().createEntity(eclass);
            scene::addNodeToContainer(modelNode, root);

            auto newEntity = Node_getEntity(modelNode);
            newEntity->setKeyValue("model", relativeModelPath);
            newEntity->setKeyValue("origin", string::to_string(modelPos));
            modelNode->assignToLayers(previousLayerSet);

            if (lastSelectedEntity)
            {
                // Preserve all spawnargs of the last selected entity, except for a few
                std::set<std::string> spawnargsToDiscard{ "model", "classname", "origin", "rotation" };

                lastSelectedEntity->forEachKeyValue([&](const std::string& key, const std::string& value)
                {
                    if (spawnargsToDiscard.count(string::to_lower_copy(key)) > 0) return;

                    rMessage() << "Replaced entity inherits the key " << key << " with value " << value << std::endl;
                    newEntity->setKeyValue(key, value);
                });
            }

            Node_setSelected(modelNode, true);
        }
        catch (cmd::ExecutionFailure& ex)
        {
            throw std::runtime_error(fmt::format(_("Unable to create model: {0}"), ex.what()));
        }
    }
    
    // It's possible that the export overwrote a model we're already using in this map, refresh it
    refreshModelsByPath(relativeModelPath);
}

void exportSelectedAsModelCmd(const cmd::ArgumentList& args)
{
	if (args.size() < 2 || args.size() > 7)
	{
		rMessage() << "Usage: ExportSelectedAsModel <Path> <ExportFormat> [<CenterObjects>] [<SkipCaulk>] [<ReplaceSelectionWithModel>] [<UseEntityOrigin>] [<ExportLightsAsObjects>]" << std::endl;
		rMessage() << "   <Path> must be an absolute file system path" << std::endl;
		rMessage() << "   pass [<CenterObjects>] as 1 to center objects around the origin" << std::endl;
		rMessage() << "   pass [<SkipCaulk>] as 1 to skip caulked surfaces" << std::endl;
		rMessage() << "   pass [<ReplaceSelectionWithModel>] as 1 to delete the selection and put the exported model in its place" << std::endl;
		rMessage() << "   pass [<UseEntityOrigin>] as 1 to use the entity origin as export origin (only applicable if a single entity is selected)" << std::endl;
		rMessage() << "   pass [<ExportLightsAsObjects>] as 1 to export lights as small polyhedric objects" << std::endl;
		return;
	}

	ModelExportOptions options;

	options.outputFilename = args[0].getString();
	options.outputFormat = args[1].getString();
	options.skipCaulk = false;
	options.centerObjects = false;
	options.replaceSelectionWithModel = false;
	options.useEntityOrigin = false;
	options.exportLightsAsObjects = false;

	if (args.size() >= 3)
	{
		options.centerObjects = (args[2].getInt() != 0);
	}

	if (args.size() >= 4)
	{
		options.skipCaulk = (args[3].getInt() != 0);
	}

	if (args.size() >= 5)
	{
		options.replaceSelectionWithModel = (args[4].getInt() != 0);
	}

	if (args.size() >= 6)
	{
		options.useEntityOrigin = (args[5].getInt() != 0);
	}

	if (args.size() >= 7)
	{
		options.exportLightsAsObjects = (args[6].getInt() != 0);
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
