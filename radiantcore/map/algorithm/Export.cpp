#include "Export.h"

#include <stdexcept>
#include "i18n.h"
#include "ieclass.h"
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
#include "command/ExecutionNotPossible.h"
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

void exportSelectedAsModel(const model::ModelExportOptions& options)
{
	if (!path_is_absolute(options.outputFilename.c_str()))
	{
		throw cmd::ExecutionNotPossible(_("Output path must be absolute."));
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

	exporter.setCenterObjects(options.exportOrigin != model::ModelExportOrigin::MapOrigin);
	exporter.setSkipCaulkMaterial(options.skipCaulk);
	exporter.setExportLightsAsObjects(options.exportLightsAsObjects);

	if (options.exportOrigin == model::ModelExportOrigin::EntityOrigin)
	{
		// Find the specified entity
        Entity* foundEntity = nullptr;

        GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
        {
            auto entity = Node_getEntity(node);
            if (!foundEntity && entity && entity->getKeyValue("name") == options.entityName)
            {
                foundEntity = entity;
            }
        });

		if (foundEntity == nullptr)
		{
            throw cmd::ExecutionFailure(fmt::format(_("Could not find the entity with name {0}"), options.entityName));
		}

		exporter.setOrigin(string::convert<Vector3>(foundEntity->getKeyValue("origin")));
        exporter.setCenterObjects(true);
	}
    else if (options.exportOrigin == model::ModelExportOrigin::CustomOrigin)
    {
        exporter.setOrigin(options.customExportOrigin);
        exporter.setCenterObjects(true);
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
		throw cmd::ExecutionFailure(_("To replace the selection with the exported model\nthe output path must be located within the mod/project."));
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
        // Place the model in the world origin, unless we set "center objects" to true
        Vector3 modelPos(0, 0, 0);

        if (options.exportOrigin != model::ModelExportOrigin::MapOrigin)
        {
            modelPos = -exporter.getCenterTransform().translation();
        }

        auto className = lastSelectedEntity ? lastSelectedEntity->getKeyValue("classname") : "func_static";
        auto eclass = GlobalEntityClassManager().findClass(className);

        if (!eclass)
        {
            throw cmd::ExecutionFailure(fmt::format(_("Cannot replace exported entity, class {0} not found"), className));
        }

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
    
    // It's possible that the export overwrote a model we're already using in this map, refresh it
    refreshModelsByPath(relativeModelPath);
}

void exportSelectedAsModelCmd(const cmd::ArgumentList& args)
{
	if (args.size() < 2 || args.size() > 8)
	{
		rMessage() << "Usage: ExportSelectedAsModel <Path> <ExportFormat> [<ExportOrigin>] [<OriginEntityName>] "
	                    "[<CustomOrigin>][<SkipCaulk>][<ReplaceSelectionWithModel>][<ExportLightsAsObjects>]" << std::endl;
		rMessage() << "   <Path> must be an absolute file system path" << std::endl;
		rMessage() << "   <ExportFormat> one of the available formats, e.g. lwo, ase, obj" << std::endl;
		rMessage() << "   [<ExportOrigin>]: 0 = Map origin, 1 = SelectionCenter, 2 = EntityOrigin, 3 = CustomOrigin" << std::endl;
		rMessage() << "   [<OriginEntityName>]: the name of the entity defining origin (if ExportOrigin == 2)" << std::endl;
		rMessage() << "   [<CustomOrigin>]: the Vector3 to be used as custom origin (if ExportOrigin == 3)" << std::endl;
		rMessage() << "   [<SkipCaulk>] as 1 to skip caulked surfaces" << std::endl;
		rMessage() << "   [<ReplaceSelectionWithModel>] as 1 to delete the selection and put the exported model in its place" << std::endl;
		rMessage() << "   [<ExportLightsAsObjects>] as 1 to export lights as small polyhedric objects" << std::endl;
		return;
	}

	model::ModelExportOptions options;

	options.outputFilename = args[0].getString();
	options.outputFormat = args[1].getString();
	options.exportOrigin = model::ModelExportOrigin::MapOrigin;
	options.entityName = std::string();
	options.customExportOrigin = Vector3(0,0,0);
	options.skipCaulk = false;
	options.replaceSelectionWithModel = false;
	options.exportLightsAsObjects = false;

	if (args.size() >= 3)
	{
		options.exportOrigin = model::getExportOriginFromString(args[2].getString());
	}

    if (args.size() >= 4)
    {
        options.entityName = args[3].getString();
    }

    if (args.size() >= 5)
    {
        options.customExportOrigin = args[4].getVector3();
    }

	if (args.size() >= 6)
	{
		options.skipCaulk = (args[5].getInt() != 0);
	}

	if (args.size() >= 7)
	{
		options.replaceSelectionWithModel = (args[6].getInt() != 0);
	}

	if (args.size() >= 8)
	{
		options.exportLightsAsObjects = (args[7].getInt() != 0);
	}

    // Forward the call, leak any ExecutionFailure exceptions
	exportSelectedAsModel(options);
}

}

}
