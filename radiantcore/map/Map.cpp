#include "Map.h"

#include "i18n.h"
#include <ostream>
#include <fstream>
#include "itextstream.h"
#include "iscenegraph.h"
#include "icameraview.h"
#include "imodel.h"
#include "igrid.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "iselectiongroup.h"
#include "ifilter.h"
#include "icounter.h"
#include "iradiant.h"
#include "imapresource.h"
#include "imapinfofile.h"
#include "iaasfile.h"
#include "igame.h"
#include "imru.h"
#include "imapformat.h"

#include "registry/registry.h"
#include "entitylib.h"
#include "gamelib.h"
#include "os/path.h"
#include "os/file.h"
#include "time/ScopeTimer.h"

#include "brush/BrushModule.h"
#include "scene/BasicRootNode.h"
#include "scene/PrefabBoundsAccumulator.h"
#include "map/MapFileManager.h"
#include "map/MapPositionManager.h"
#include "map/MapResource.h"
#include "map/algorithm/Import.h"
#include "map/algorithm/Export.h"
#include "scene/Traverse.h"
#include "map/algorithm/MapExporter.h"
#include "model/export/ModelExporter.h"
#include "model/export/ModelScalePreserver.h"
#include "messages/ScopedLongRunningOperation.h"
#include "messages/FileOverwriteConfirmation.h"
#include "messages/FileSaveConfirmation.h"
#include "messages/MapOperationMessage.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Group.h"
#include "scene/Group.h"
#include "selection/algorithm/Transformation.h"
#include "module/StaticModule.h"
#include "command/ExecutionNotPossible.h"
#include "MapPropertyInfoFileModule.h"
#include "messages/NotificationMessage.h"

#include <fmt/format.h>

#include "messages/MapFileOperation.h"
#include "scene/ChildPrimitives.h"
#include "scene/merge/GraphComparer.h"
#include "scene/merge/MergeOperation.h"
#include "scene/merge/ThreeWayMergeOperation.h"
#include "scene/merge/MergeLib.h"

namespace map
{

namespace
{
    const char* const MAP_UNNAMED_STRING = N_("unnamed.map");
}

Map::Map() :
    _lastCopyMapName(""),
    _modified(false),
    _saveInProgress(false),
    _shutdownListener(0)
{}

void Map::clearMapResource()
{
    // Map is unnamed or load failed, reset map resource node to empty
    _resource->clear();

    _resource->getRootNode()->getUndoChangeTracker().setSavedChangeCount();

    // Rename the map to "unnamed" in any case to avoid overwriting the failed map
    setMapName(_(MAP_UNNAMED_STRING));
}

void Map::connectToRootNode()
{
    _modifiedStatusListener.disconnect();
    _undoEventListener.disconnect();
    _layerHierarchyChangedListener.disconnect();

    _modifiedStatusListener = _resource->signal_modifiedStatusChanged().connect(
        [this](bool newStatus) { setModified(newStatus); }
    );

    if (!_resource->getRootNode()) return;

    _undoEventListener = _resource->getRootNode()->getUndoSystem().signal_undoEvent().connect(
        sigc::mem_fun(*this, &Map::onUndoEvent)
    );

    // This is a workaround - changing layer hierarchies is not an undoable operation
    // and this by hitting undo or redo the status might be reset to "unmodified" anytime
    _layerHierarchyChangedListener = _resource->getRootNode()->getLayerManager()
        .signal_layerHierarchyChanged().connect(sigc::mem_fun(*this, &Map::onLayerHierarchyChanged));
}

void Map::onLayerHierarchyChanged()
{
    setModified(true);
}

void Map::onUndoEvent(IUndoSystem::EventType type, const std::string& operationName)
{
    switch (type)
    {
    case IUndoSystem::EventType::OperationRecorded:
        OperationMessage::Send(operationName);
        break;

    case IUndoSystem::EventType::OperationUndone:
        _mapPostUndoSignal.emit();
        OperationMessage::Send(fmt::format(_("Undo: {0}"), operationName));
        break;

    case IUndoSystem::EventType::OperationRedone:
        _mapPostRedoSignal.emit();
        OperationMessage::Send(fmt::format(_("Redo: {0}"), operationName));
        break;
    }
}

void Map::loadMapResourceFromPath(const std::string& path)
{
    // Create a MapLocation defining a physical file, and forward the call
    loadMapResourceFromLocation(MapLocation{path, false, ""});
}

void Map::loadMapResourceFromArchive(const std::string& archive, const std::string& archiveRelativePath)
{
    // Create a MapLocation defining an archive file, and forward the call
    loadMapResourceFromLocation(MapLocation{ archive, true, archiveRelativePath });
}

void Map::loadMapResourceFromLocation(const MapLocation& location)
{
    rMessage() << "Loading map from " << location.path <<
        (location.isArchive ? " [" + location.archiveRelativePath + "]" : "") << std::endl;

	// Map loading started
	emitMapEvent(MapLoading);

    // Abort any ongoing merge
    abortMergeOperation();

	_resource = location.isArchive ?
        GlobalMapResourceManager().createFromArchiveFile(location.path, location.archiveRelativePath) :
        GlobalMapResourceManager().createFromPath(location.path);

    assert(_resource);

    try
    {
        util::ScopeTimer timer("map load");

        if (isUnnamed() || !_resource->load())
        {
            clearMapResource();
        }
    }
    catch (const IMapResource::OperationException& ex)
    {
        radiant::NotificationMessage::SendError(ex.what());
        clearMapResource();
    }

    connectToRootNode();

    // Take the new node and insert it as map root
    GlobalSceneGraph().setRoot(_resource->getRootNode());

	// Traverse the scenegraph and find the worldspawn
	findWorldspawn();

    // Associate the Scenegaph with the global RenderSystem
    // This usually takes a while since all editor textures are loaded - display a dialog to inform the user
    {
        radiant::ScopedLongRunningOperation blocker(_("Loading textures..."));

        GlobalSceneGraph().root()->setRenderSystem(std::dynamic_pointer_cast<RenderSystem>(
            module::GlobalModuleRegistry().getModule(MODULE_RENDERSYSTEM)));
    }

    // Update layer visibility of all nodes
    scene::UpdateNodeVisibilityWalker updater(_resource->getRootNode()->getLayerManager());
    _resource->getRootNode()->traverse(updater);

    // Map loading finished, emit the signal
    emitMapEvent(MapLoaded);

    OperationMessage::Send(_("Map loaded"));

    rMessage() << "--- LoadMapFile ---\n";
    rMessage() << _mapName << "\n";

    rMessage() << GlobalCounters().getCounter(counterBrushes).get() << " brushes\n";
    rMessage() << GlobalCounters().getCounter(counterPatches).get() << " patches\n";
    rMessage() << GlobalCounters().getCounter(counterEntities).get() << " entities\n";

    // Let the filtersystem update the filtered status of all instances
    GlobalFilterSystem().update();

    // Clear the modified flag
    setModified(false);
}

void Map::finishMergeOperation()
{
    if (getEditMode() != EditMode::Merge)
    {
        rWarning() << "Not in merge edit mode, cannot finish any operation" << std::endl;
        return;
    }

    if (!_mergeOperation)
    {
        rError() << "Cannot merge, no active operation attached to this map." << std::endl;
        return;
    }

    // Prepare the scene, let the merge nodes know about the upcoming merge
    // and remove them from the scene, to leave it untouched
    for (const auto& mergeActionNode : _mergeActionNodes)
    {
        mergeActionNode->prepareForMerge();

        scene::removeNodeFromParent(mergeActionNode);

        // Clear any references this node holds
        mergeActionNode->clear();
    }

    _mergeActionNodes.clear();

    // At this point the scene should look the same as before the merge
    {
        UndoableCommand cmd("mergeMap");
        _mergeOperation->applyActions();

        cleanupMergeOperation();
    }
    setEditMode(EditMode::Normal);
    emitMapEvent(IMap::MapMergeOperationFinished);
}

void Map::cleanupMergeOperation()
{
    for (const auto& mergeActionNode : _mergeActionNodes)
    {
        // If the node is already removed from the scene, this does nothing
        scene::removeNodeFromParent(mergeActionNode);

        // Clear any references this node holds
        mergeActionNode->clear();
    }

    _mergeOperationListener.disconnect();
    _mergeActionNodes.clear();
    _mergeOperation.reset();
}

void Map::abortMergeOperation()
{
    bool mergeWasActive = _mergeOperation != nullptr;

    // Remove the nodes and switch back to normal without applying the operation
    cleanupMergeOperation();
    setEditMode(EditMode::Normal);

    if (mergeWasActive)
    {
        emitMapEvent(IMap::MapMergeOperationAborted);
    }
}

scene::merge::IMergeOperation::Ptr Map::getActiveMergeOperation()
{
    return _editMode == EditMode::Merge ? _mergeOperation : scene::merge::IMergeOperation::Ptr();
}

void Map::setMapName(const std::string& newName)
{
    bool mapNameChanged = _mapName != newName;

    // Store the name into the member
    _mapName = newName;

    // Update the map resource's root node, if there is one
    if (_resource)
	{
        _resource->rename(newName);
    }

    if (mapNameChanged)
    {
        // Fire the signal to any observers
        signal_mapNameChanged().emit();
    }
}

sigc::signal<void>& Map::signal_mapNameChanged()
{
    return _mapNameChangedSignal;
}

std::string Map::getMapName() const {
    return _mapName;
}

bool Map::isUnnamed() const {
    return _mapName == _(MAP_UNNAMED_STRING);
}

namespace
{

bool pointfileNameMatch(const std::string& candidate, const std::string& mapStem)
{
    // A matching point file either has an identical stem to the map file, or the map file stem
    // with an underscore suffix (e.g. "mapfile_portal_123_456.lin")
    return string::iequals(candidate, mapStem) || string::istarts_with(candidate, mapStem + "_");
}

} // namespace

void Map::forEachPointfile(PointfileFunctor func) const
{
    static const char* LIN_EXT = ".lin";

    const fs::path map(getMapName());
    const fs::path mapDir = map.parent_path();
    const fs::path mapStem = map.stem();

    // Don't bother trying to iterate over a missing map directory, this will
    // just throw an exception.
    if (!fs::is_directory(mapDir))
        return;

    // Iterate over files in the map directory, putting them in a sorted set
    std::set<fs::path> paths;
    for (const auto& entry: fs::directory_iterator(mapDir))
    {
        // Ignore anything which isn't a .lin file
        auto entryPath = entry.path();
        if (entryPath.extension() == LIN_EXT
            && pointfileNameMatch(entryPath.stem().string(), mapStem.string()))
        {
            paths.insert(entryPath);
        }
    }

    // Call functor on paths in order
    for (const fs::path& p: paths)
        func(p);
}

void Map::showPointFile(const fs::path& filePath)
{
    _pointTrace->show(filePath);
}

bool Map::isPointTraceVisible() const
{
    return _pointTrace->isVisible();
}

void Map::onSceneNodeErase(const scene::INodePtr& node)
{
	// Detect when worldspawn is removed from the map
	if (node == _worldSpawnNode)
	{
		_worldSpawnNode.reset();
	}
}

void Map::setWorldspawn(const scene::INodePtr& node)
{
    _worldSpawnNode = node;
}

Map::MapEventSignal Map::signal_mapEvent() const
{
	return _mapEvent;
}

Map::EditMode Map::getEditMode()
{
    return _editMode;
}

void Map::setEditMode(EditMode mode)
{
    _editMode = mode;

    if (_editMode == EditMode::Merge)
    {
        GlobalSelectionSystem().setSelectedAll(false);
        GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::MergeAction);

        if (getRoot())
        {
            getRoot()->getRenderSystem()->setMergeModeEnabled(true);
        }
    }
    else
    {
        GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::Primitive);

        if (getRoot())
        {
            getRoot()->getRenderSystem()->setMergeModeEnabled(false);
        }
    }

    signal_editModeChanged().emit(_editMode);
    SceneChangeNotify();
}

sigc::signal<void, IMap::EditMode>& Map::signal_editModeChanged()
{
    return _mapEditModeChangedSignal;
}

const scene::INodePtr& Map::getWorldspawn()
{
    return _worldSpawnNode;
}

scene::IMapRootNodePtr Map::getRoot()
{
    if (_resource)
	{
        // Try to cast the node onto a root node and return
        return _resource->getRootNode();
    }

    return scene::IMapRootNodePtr();
}

MapFormatPtr Map::getFormat()
{
    return GlobalMapFormatManager().getMapFormatForFilename(_mapName);
}

MapFormatPtr Map::getMapFormatForFilenameSafe(const std::string& filename)
{
    auto candidate = GlobalMapFormatManager().getMapFormatForFilename(filename);

    // Fall back to the format of the current map if the selection is empty (#5808)
    return candidate ? candidate : getFormat();
}

// free all map elements, reinitialize the structures that depend on them
void Map::freeMap()
{
    // Abort any ongoing merge
    abortMergeOperation();

	// Fire the map unloading event,
	// This will de-select stuff, clear the pointfile, etc.
    emitMapEvent(MapUnloading);

	setWorldspawn(scene::INodePtr());

	GlobalSceneGraph().setRoot(scene::IMapRootNodePtr());

    emitMapEvent(MapUnloaded);

    // Reset the resource pointer
    _modifiedStatusListener.disconnect();
    _resource.reset();
}

bool Map::isModified() const
{
    return _modified;
}

void Map::setModified(bool modifiedFlag)
{
    if (_modified != modifiedFlag)
    {
        _modified = modifiedFlag;

        // when the map is modified, let the listeners now
        signal_modifiedChanged().emit();
    }

    // Reset the map save timer
    _mapSaveTimer.restart();
}

sigc::signal<void>& Map::signal_modifiedChanged()
{
    return _mapModifiedChangedSignal;
}

sigc::signal<void>& Map::signal_postUndo()
{
    return _mapPostUndoSignal;
}

sigc::signal<void>& Map::signal_postRedo()
{
    return _mapPostRedoSignal;
}

IUndoSystem& Map::getUndoSystem()
{
    const auto& rootNode = _resource->getRootNode();

    if (!rootNode)
    {
        throw std::runtime_error("No map resource loaded");
    }

    return rootNode->getUndoSystem();
}

// move the view to a certain position
void Map::focusViews(const Vector3& point, const Vector3& angles)
{
    // Set the camera and the views to the given point
    GlobalCameraManager().focusAllCameras(point, angles);

    // ortho views might not be present in headless mode
    if (module::GlobalModuleRegistry().moduleExists(MODULE_ORTHOVIEWMANAGER))
    {
        GlobalXYWndManager().setOrigin(point);
    }
}

void Map::focusViewCmd(const cmd::ArgumentList& args)
{
    if (args.size() != 2)
    {
        rWarning() << "Usage: FocusViews <origin:Vector3> <angles:Vector3>" << std::endl;
        return;
    }

    focusViews(args[0].getVector3(), args[1].getVector3());
}

void Map::focusCameraOnSelectionCmd(const cmd::ArgumentList& args)
{
    if (GlobalSelectionSystem().countSelected() == 0)
    {
        throw cmd::ExecutionNotPossible(_("Cannot focus, selection is empty"));
    }

    // Determine the bounds of the current selection
    const auto& workZone = GlobalSelectionSystem().getWorkZone();
    auto originAndAngles = scene::getOriginAndAnglesToLookAtBounds(workZone.bounds);

    // Set the camera and the views to the given point
    GlobalCameraManager().focusAllCameras(originAndAngles.first, originAndAngles.second);
}

scene::INodePtr Map::findWorldspawn()
{
	scene::INodePtr worldspawn;

    // Traverse the scenegraph and search for the worldspawn
	GlobalSceneGraph().root()->foreachNode([&](const scene::INodePtr& node)
	{
		if (Node_isWorldspawn(node))
		{
			worldspawn = node;
			return false; // done traversing
		}

		return true;
	});

	// Set the worldspawn, might be null if nothing was found
	setWorldspawn(worldspawn);

    return worldspawn;
}

scene::INodePtr Map::createWorldspawn()
{
	scene::INodePtr worldspawn(GlobalEntityModule().createEntity(
		GlobalEntityClassManager().findOrInsert("worldspawn", true)));

	// We want the world spawn entity to go for the pole position
	GlobalSceneGraph().root()->addChildNodeToFront(worldspawn);

	return worldspawn;
}

const scene::INodePtr& Map::findOrInsertWorldspawn()
{
	// If we don't know any worldspawn yet, and can't find one either,
	// let's create one afresh
	if (!_worldSpawnNode && findWorldspawn() == nullptr)
	{
		setWorldspawn(createWorldspawn());
	}

    return _worldSpawnNode;
}

void Map::load(const std::string& filename)
{
    setMapName(filename);
    loadMapResourceFromPath(_mapName);
}

bool Map::save(const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return false; // safeguard

    if (_resource->isReadOnly())
    {
        rError() << "This map is read-only and cannot be saved." << std::endl;
        return false;
    }

    // Check if the map file has been modified in the meantime
    if (_resource->fileOnDiskHasBeenModifiedSinceLastSave() &&
        !radiant::FileOverwriteConfirmation::SendAndReceiveAnswer(
            fmt::format(_("The file {0} has been modified since it was last saved,\nperhaps by another application. "
                "Do you really want to overwrite the file?"), _mapName), _("File modification detected")))
    {
        return false;
    }

    _saveInProgress = true;

    emitMapEvent(MapSaving);

    util::ScopeTimer timer("map save");

    bool success = false;

    // Save the actual map resource
    try
    {
        _resource->save(mapFormat);

        // Clear the modified flag
        setModified(false);

        success = true;
    }
    catch (IMapResource::OperationException & ex)
    {
        radiant::NotificationMessage::SendError(ex.what());
    }

    emitMapEvent(MapSaved);
    OperationMessage::Send(_("Map saved"));

    _saveInProgress = false;

    // Redraw the views, sometimes the backbuffer containing
    // the previous frame will remain visible
    SceneChangeNotify();

    return success;
}

void Map::createNewMap()
{
    setMapName(_(MAP_UNNAMED_STRING));

	loadMapResourceFromPath(_mapName);

    SceneChangeNotify();

    setModified(false);

    OperationMessage::Send(_("Empty Map created"));
    focusViews(Vector3(0,0,30), Vector3(0,0,0));
}

IMapExporter::Ptr Map::createMapExporter(IMapWriter& writer,
    const scene::IMapRootNodePtr& root, std::ostream& mapStream)
{
    return std::make_shared<MapExporter>(writer, root, mapStream, 0);
}

bool Map::import(const std::string& filename)
{
    bool success = false;

    IMapResourcePtr resource = GlobalMapResourceManager().createFromPath(filename);

    try
    {
        if (resource->load())
        {
            // load() returned TRUE, this means that the resource node
            // is not the NULL node
            const auto& otherRoot = resource->getRootNode();

            // Adjust all new names to fit into the existing map namespace
            algorithm::prepareNamesForImport(getRoot(), otherRoot);

            algorithm::importMap(otherRoot);
            success = true;
        }

        SceneChangeNotify();
    }
    catch (const IMapResource::OperationException& ex)
    {
        radiant::NotificationMessage::SendError(ex.what());
    }

    return success;
}

void Map::saveDirect(const std::string& filename, const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return; // safeguard

    _saveInProgress = true;

	MapFormatPtr format = mapFormat;

	if (!mapFormat)
	{
		format = getMapFormatForFilenameSafe(filename);
	}

    try
    {
        MapResource::saveFile(*format, GlobalSceneGraph().root(), scene::traverse, filename);
    }
    catch (const IMapResource::OperationException& ex)
    {
        radiant::NotificationMessage::SendError(ex.what());
    }

    _saveInProgress = false;
}

void Map::saveSelected(const std::string& filename, const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return; // safeguard

    _saveInProgress = true;

	MapFormatPtr format = mapFormat;

	if (!format)
	{
		format = getMapFormatForFilenameSafe(filename);
	}

    try
    {
        MapResource::saveFile(
            *format,
            GlobalSceneGraph().root(),
            scene::traverseSelected, // TraversalFunc
            filename
        );
    }
    catch (const IMapResource::OperationException& ex)
    {
        radiant::NotificationMessage::SendError(ex.what());
    }

    _saveInProgress = false;
}

std::string Map::getSaveConfirmationText() const
{
    std::string primaryText = fmt::format(_("Save changes to map \"{0}\"\nbefore closing?"), _mapName);

    // Display "x seconds" or "x minutes"
    int seconds = static_cast<int>(_mapSaveTimer.getSecondsPassed());
    std::string timeString;
    if (seconds > 120)
    {
        timeString = fmt::format(_("{0:d} minutes"), (seconds / 60));
    }
    else
    {
        timeString = fmt::format(_("{0:d} seconds"), seconds);
    }

    std::string secondaryText = fmt::format(
        _("If you don't save, changes from the last {0}\nwill be lost."), timeString);

    std::string confirmText = fmt::format("{0}\n\n{1}", primaryText, secondaryText);

    return confirmText;
}

bool Map::askForSave(const std::string& title)
{
    if (!isModified())
    {
        // Map is not modified, return positive
        return true;
    }

    // Ask the user
    auto answer = radiant::FileSaveConfirmation::SendAndReceiveAnswer(getSaveConfirmationText(), title);

    if (answer == radiant::FileSaveConfirmation::Action::Cancel)
    {
        return false;
    }

    if (answer == radiant::FileSaveConfirmation::Action::SaveChanges)
    {
        // The user wants to save the map
        if (isUnnamed())
        {
            // Map still unnamed, try to save the map with a new name
            // and take the return value from the other routine.
            return saveAs();
        }
        else
        {
            // Map is named, save it
            save();
        }
    }

    // Default behaviour: allow the action
    return true;
}

bool Map::saveAs()
{
    if (_saveInProgress) return false; // safeguard

    auto fileInfo = MapFileManager::getMapFileSelection(false,
        _("Save Map"), filetype::TYPE_MAP, getMapName());

    if (fileInfo.fullPath.empty())
    {
        // Invalid filename entered, return false
        return false;
    }

    // Remember the old resource, we might need to revert
    auto oldResource = _resource;

    // Create a new resource pointing to the given path...
    _resource = GlobalMapResourceManager().createFromPath(fileInfo.fullPath);

    // ...and import the existing root node into that resource
    _resource->setRootNode(oldResource->getRootNode());

    // Try to save the resource, this might fail
    if (!save(fileInfo.mapFormat))
    {
        // Failure, revert the change
        _resource = oldResource;
        return false;
    }

    connectToRootNode();

    // Resource save was successful, notify about this name change
    rename(fileInfo.fullPath);

    // add an MRU entry on success
    GlobalMRU().insert(fileInfo.fullPath);

    return true;
}

void Map::saveCopyAs()
{
    // Let's see if we can remember a map name from a previous save
    if (_lastCopyMapName.empty())
    {
        _lastCopyMapName = getMapName();
    }

	auto fileInfo = MapFileManager::getMapFileSelection(false,
        _("Save Copy As..."), filetype::TYPE_MAP, _lastCopyMapName);

	if (!fileInfo.fullPath.empty())
	{
        saveCopyAs(fileInfo.fullPath, fileInfo.mapFormat);
    }
}

void Map::saveCopyAs(const std::string& absolutePath, const MapFormatPtr& mapFormat)
{
    if (absolutePath.empty())
    {
        rWarning() << "Map::saveCopyAs: path must not be empty" << std::endl;
        return;
    }

    // Remember the last name
    _lastCopyMapName = absolutePath;

    // Return the result of the actual save method
    saveDirect(absolutePath, mapFormat);
}

void Map::loadPrefabAt(const cmd::ArgumentList& args)
{
    if (args.size() < 2 || args.size() > 4)
    {
        rWarning() << "Usage: " << LOAD_PREFAB_AT_CMD <<
            " <prefabPath:String> <targetCoords:Vector3> [insertAsGroup:0|1] [recalculatePrefabOrigin:0|1]" << std::endl;
        return;
    }

    auto prefabPath = args[0].getString();
    auto targetCoords = args[1].getVector3();
    auto insertAsGroup = args.size() > 2 ? args[2].getBoolean() : false;
    auto recalculatePrefabOrigin = args.size() > 3 ? args[3].getBoolean() : true;

	if (!prefabPath.empty())
	{
        UndoableCommand undo("loadPrefabAt");

        // Deselect everything
        GlobalSelectionSystem().setSelectedAll(false);

        // Now import the prefab (imported items get selected)
		import(prefabPath);

        // Get the selection bounds, snap its origin to the grid
        scene::PrefabBoundsAccumulator accumulator;
        GlobalSelectionSystem().foreachSelected(accumulator);

        if (recalculatePrefabOrigin)
        {
            auto prefabCenter = accumulator.getBounds().getOrigin().getSnapped(GlobalGrid().getGridSize());

            // Switch texture lock on
            bool prevTexLockState = GlobalBrush().textureLockEnabled();
            GlobalBrush().setTextureLock(true);

            // Translate the selection to the given point
            selection::algorithm::translateSelected(targetCoords - prefabCenter);

            // Revert to previous state
            GlobalBrush().setTextureLock(prevTexLockState);
        }

		// Check whether we should group the prefab parts
		if (insertAsGroup && GlobalSelectionSystem().countSelected() > 1)
		{
			try
			{
				selection::groupSelected();
			}
			catch (cmd::ExecutionNotPossible& ex)
			{
				// Ignore grouping errors on prefab insert, just log the message
				rError() << "Error grouping the prefab: " << ex.what() << std::endl;
			}
		}
    }
}

void Map::saveMapCopyAs(const cmd::ArgumentList& args)
{
    if (args.size() == 0 || args[0].getString().empty())
    {
        // Use the overload without arguments, it will ask for a file name
        saveCopyAs();
    }
    else
    {
        // Pass the first argument we got
        saveCopyAs(args[0].getString());
    }
}

void Map::saveAutomaticMapBackup(const cmd::ArgumentList& args)
{
    // Use the saveDirect routine to not change with the _lastCopyMapName member
    saveDirect(args[0].getString());
}

void Map::registerCommands()
{
    GlobalCommandSystem().addCommand("NewMap", Map::newMap);
    GlobalCommandSystem().addCommand("OpenMap", std::bind(&Map::openMapCmd, this, std::placeholders::_1), 
        { cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL });
    GlobalCommandSystem().addCommand("OpenMapFromArchive", Map::openMapFromArchive, { cmd::ARGTYPE_STRING, cmd::ARGTYPE_STRING });
    GlobalCommandSystem().addCommand("ImportMap", Map::importMap);
    GlobalCommandSystem().addCommand("StartMergeOperation", std::bind(&Map::startMergeOperationCmd, this, std::placeholders::_1),
        { cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL });
    GlobalCommandSystem().addCommand("AbortMergeOperation", std::bind(&Map::abortMergeOperationCmd, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("FinishMergeOperation", std::bind(&Map::finishMergeOperationCmd, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand(LOAD_PREFAB_AT_CMD, std::bind(&Map::loadPrefabAt, this, std::placeholders::_1),
        { cmd::ARGTYPE_STRING, cmd::ARGTYPE_VECTOR3, cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL });
    GlobalCommandSystem().addCommand("SaveSelectedAsPrefab", Map::saveSelectedAsPrefab);
    GlobalCommandSystem().addCommand("SaveMap", std::bind(&Map::saveMapCmd, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("SaveMapAs", Map::saveMapAs);
    GlobalCommandSystem().addCommand("SaveMapCopyAs", std::bind(&Map::saveMapCopyAs, this, std::placeholders::_1), { cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL });
    // Command used by the autosaver to save a copy without messing with the remembered paths
    GlobalCommandSystem().addCommand("SaveAutomaticBackup", std::bind(&Map::saveAutomaticMapBackup, this, std::placeholders::_1), { cmd::ARGTYPE_STRING });
    GlobalCommandSystem().addCommand("ExportMap", std::bind(&Map::exportMap, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("SaveSelected", Map::exportSelection);
    GlobalCommandSystem().addCommand("FocusViews", std::bind(&Map::focusViewCmd, this, std::placeholders::_1), { cmd::ARGTYPE_VECTOR3, cmd::ARGTYPE_VECTOR3 });
    GlobalCommandSystem().addCommand("FocusCameraViewOnSelection", std::bind(&Map::focusCameraOnSelectionCmd, this, std::placeholders::_1));
    // ExportSelectedAsModel <Path> <ExportFormat> [<ExportOrigin>] [<OriginEntityName>] [<CustomOrigin>] [<SkipCaulk>] [<ReplaceSelectionWithModel>] [<ExportLightsAsObjects>]
	GlobalCommandSystem().addCommand("ExportSelectedAsModel", algorithm::exportSelectedAsModelCmd,
        { cmd::ARGTYPE_STRING, // path
          cmd::ARGTYPE_STRING, // export format
          cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL, // export origin type
          cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL, // origin entity name
          cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_OPTIONAL, // custom origin
          cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL, // skip caulk
          cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL, // replace selection with model
          cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL }); // export lights as objects

    // Add undo commands
    GlobalCommandSystem().addCommand("Undo", std::bind(&Map::undoCmd, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("Redo", std::bind(&Map::redoCmd, this, std::placeholders::_1));
}

void Map::undoCmd(const cmd::ArgumentList& args)
{
    try
    {
        getUndoSystem().undo();
    }
    catch (const std::runtime_error& err)
    {
        throw cmd::ExecutionNotPossible(err.what());
    }
}

void Map::redoCmd(const cmd::ArgumentList& args)
{
    try
    {
        getUndoSystem().redo();
    }
    catch (const std::runtime_error& err)
    {
        throw cmd::ExecutionNotPossible(err.what());
    }
}

// Static command targets
void Map::newMap(const cmd::ArgumentList& args)
{
    if (GlobalMap().askForSave(_("New Map")))
	{
        GlobalMap().freeMap();
        GlobalMap().createNewMap();
    }
}

void Map::openMapCmd(const cmd::ArgumentList& args)
{
    if (!askForSave(_("Open Map"))) return;

    std::string candidate;

    if (!args.empty())
    {
        candidate = args[0].getString();
    }
    else
    {
        // No arguments passed, get the map file name to load
        auto fileInfo = MapFileManager::getMapFileSelection(true, _("Open map"), filetype::TYPE_MAP);
        candidate = fileInfo.fullPath;
    }

    std::string mapToLoad;

    if (os::fileOrDirExists(candidate))
    {
        mapToLoad = candidate;
    }
    else if (!candidate.empty())
    {
        // Try to open this file from the VFS (this will hit physical files
        // in the active project as well as files in registered PK4)
        if (GlobalFileSystem().openTextFile(candidate))
        {
            mapToLoad = candidate;
        }
        else
        {
            // Next, try to look up the map in the regular maps path
            fs::path mapsPath = GlobalGameManager().getMapPath();
            fs::path fullMapPath = mapsPath / candidate;

            if (os::fileOrDirExists(fullMapPath.string()))
            {
                mapToLoad = fullMapPath.string();
            }
            else
            {
                throw cmd::ExecutionFailure(fmt::format(_("File doesn't exist: {0}"), candidate));
            }
        }
    }

    if (!mapToLoad.empty())
	{
        GlobalMRU().insert(mapToLoad);

        freeMap();
        load(mapToLoad);
    }
}

void Map::openMapFromArchive(const cmd::ArgumentList& args)
{
    if (args.size() != 2)
    {
        rWarning() << "Usage: OpenMapFromArchive <pathToPakFile> <pathWithinArchive>" << std::endl;
        return;
    }

    if (!GlobalMap().askForSave(_("Open Map"))) return;

    std::string pathToArchive = args[0].getString();
    std::string relativePath = args[1].getString();

    if (!os::fileOrDirExists(pathToArchive))
    {
        throw cmd::ExecutionFailure(fmt::format(_("File not found: {0}"), pathToArchive));
    }

    if (!pathToArchive.empty())
    {
        GlobalMap().freeMap();
        GlobalMap().setMapName(relativePath);
        GlobalMap().loadMapResourceFromArchive(pathToArchive, relativePath);
    }
}

void Map::importMap(const cmd::ArgumentList& args)
{
    MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(true, _("Import map"), filetype::TYPE_MAP);

    if (!fileInfo.fullPath.empty())
    {
        UndoableCommand undo("mapImport");
        GlobalMap().import(fileInfo.fullPath);
    }
}

void Map::saveMapAs(const cmd::ArgumentList& args) {
    GlobalMap().saveAs();
}

void Map::saveMapCmd(const cmd::ArgumentList& args)
{
    if (isUnnamed() || (_resource && _resource->isReadOnly()))
    {
        saveAs();
    }
    // greebo: Always let the map be saved, regardless of the modified status.
    else /*if(GlobalMap().isModified())*/
    {
        save();
    }
}

void Map::exportMap(const cmd::ArgumentList& args)
{
    auto fileInfo = MapFileManager::getMapFileSelection(false, _("Export Map"), filetype::TYPE_MAP_EXPORT);

    if (!fileInfo.fullPath.empty())
	{
        if (!fileInfo.mapFormat)
        {
            fileInfo.mapFormat = getMapFormatForFilenameSafe(fileInfo.fullPath);
        }

        emitMapEvent(MapSaving);

        MapResource::saveFile(*fileInfo.mapFormat,
            GlobalSceneGraph().root(),
            scene::traverse,
            fileInfo.fullPath);

        emitMapEvent(MapSaved);
    }
}

void Map::exportSelection(const cmd::ArgumentList& args)
{
    MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(false, _("Export selection"), filetype::TYPE_MAP);

    if (!fileInfo.fullPath.empty())
	{
		GlobalMap().saveSelected(fileInfo.fullPath, fileInfo.mapFormat);
    }
}

void Map::saveSelectedAsPrefab(const cmd::ArgumentList& args)
{
    MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(false, _("Save selected as Prefab"), filetype::TYPE_PREFAB);

	if (!fileInfo.fullPath.empty())
    {
        GlobalMap().saveSelected(fileInfo.fullPath, fileInfo.mapFormat);
    }
}

void Map::rename(const std::string& filename)
{
    if (_mapName != filename)
    {
        setMapName(filename);
        SceneChangeNotify();
    }
    else
    {
        _resource->save();
        setModified(false);
    }
}

void Map::exportSelected(std::ostream& out)
{
    exportSelected(out, getFormat());
}

void Map::exportSelected(std::ostream& out, const MapFormatPtr& format)
{
    assert(format);

    // Create our main MapExporter walker for traversal
    auto writer = format->getMapWriter();
    
    try
    {
        MapExporter exporter(*writer, GlobalSceneGraph().root(), out);
        exporter.disableProgressMessages();

        // Pass the traverseSelected function and start writing selected nodes
        exporter.exportMap(GlobalSceneGraph().root(), scene::traverseSelected);
    }
    catch (FileOperation::OperationCancelled&)
    {
        radiant::NotificationMessage::SendInformation(_("Map export cancelled"));
    }
}

void Map::onMergeActionAdded(const scene::merge::IMergeAction::Ptr& action)
{
    UndoableCommand cmd("insertMergeAction");

    _mergeActionNodes.emplace_back(std::make_shared<scene::RegularMergeActionNode>(action));
    getRoot()->addChildNode(_mergeActionNodes.back());
}

void Map::createMergeActions()
{
    // Group spawnarg actions into one single node if applicable
    std::map<scene::INodePtr, std::vector<scene::merge::IMergeAction::Ptr>> entityChanges;
    std::vector<scene::merge::IMergeAction::Ptr> otherChanges;

    _mergeOperation->foreachAction([&](const scene::merge::IMergeAction::Ptr& action)
    {
        if (scene::merge::actionIsTargetingKeyValue(action))
        {
            auto& actions = entityChanges.try_emplace(action->getAffectedNode()).first->second;
            actions.push_back(action);
        }
        else // regular change, add it to the misc pile
        {
            otherChanges.push_back(action);
        }
    });

    _mergeOperationListener = _mergeOperation->sig_ActionAdded().connect(sigc::mem_fun(this, &Map::onMergeActionAdded));

    UndoableCommand cmd("createMergeOperation");

    // Construct all entity changes...
    for (const auto& pair : entityChanges)
    {
        _mergeActionNodes.emplace_back(std::make_shared<scene::KeyValueMergeActionNode>(pair.second));
        getRoot()->addChildNode(_mergeActionNodes.back());
    }

    // ...and the regular ones
    for (const auto& action : otherChanges)
    {
        _mergeActionNodes.emplace_back(std::make_shared<scene::RegularMergeActionNode>(action));
        getRoot()->addChildNode(_mergeActionNodes.back());
    }
}

void Map::prepareMergeOperation()
{
    if (!getRoot())
    {
        throw cmd::ExecutionNotPossible(_("No map loaded, cannot merge"));
    }

    {
        // Make sure we have a worldspawn in this map
        UndoableCommand cmd("ensureWorldSpawn");
        findOrInsertWorldspawn();
    }

    // Stop any pending merge operation
    abortMergeOperation();
}

void Map::startMergeOperation(const std::string& sourceMap)
{
    if (!os::fileOrDirExists(sourceMap))
    {
        throw cmd::ExecutionFailure(fmt::format(_("File doesn't exist: {0}"), sourceMap));
    }

    prepareMergeOperation();

    auto sourceMapResource = GlobalMapResourceManager().createFromPath(sourceMap);

    try
    {
        if (sourceMapResource->load())
        {
            // Compare the scenes and get the report
            auto result = scene::merge::GraphComparer::Compare(sourceMapResource->getRootNode(), getRoot());

            // Create the merge actions
            _mergeOperation = scene::merge::MergeOperation::CreateFromComparisonResult(*result);

            if (_mergeOperation->hasActions())
            {
                // Create renderable merge actions
                createMergeActions();

                // Switch to merge mode
                setEditMode(EditMode::Merge);

                emitMapEvent(IMap::MapMergeOperationStarted);
            }
            else
            {
                radiant::NotificationMessage::SendInformation(_("The Merge Operation turns out to be empty, nothing to do."));
            }

            // Dispose of the resource, we don't need it anymore
            sourceMapResource->clear();
        }
    }
    catch (const IMapResource::OperationException& ex)
    {
        radiant::NotificationMessage::SendError(ex.what());
    }
}

void Map::startMergeOperation(const std::string& sourceMap, const std::string& baseMap)
{
    prepareMergeOperation();

    auto baseMapResource = GlobalMapResourceManager().createFromPath(baseMap);
    auto sourceMapResource = GlobalMapResourceManager().createFromPath(sourceMap);

    try
    {
        if (sourceMapResource->load() && baseMapResource->load())
        {
            _mergeOperation = scene::merge::ThreeWayMergeOperation::Create(
                baseMapResource->getRootNode(), sourceMapResource->getRootNode(), getRoot());

            if (_mergeOperation->hasActions())
            {
                // Create renderable merge actions
                createMergeActions();

                // Switch to merge mode
                setEditMode(EditMode::Merge);

                emitMapEvent(IMap::MapMergeOperationStarted);
            }
            else
            {
                radiant::NotificationMessage::SendInformation(_("The Merge Operation turns out to be empty, nothing to do."));
            }

            // Dispose of the resources, we don't need it anymore
            sourceMapResource->clear();
            baseMapResource->clear();
        }
    }
    catch (const IMapResource::OperationException& ex)
    {
        radiant::NotificationMessage::SendError(ex.what());
    }
}

void Map::startMergeOperationCmd(const cmd::ArgumentList& args)
{
    if (!getRoot())
    {
        throw cmd::ExecutionNotPossible(_("No map loaded, cannot merge"));
    }

    std::string sourceCandidate;
    std::string baseCandidate;

    if (!args.empty())
    {
        sourceCandidate = args[0].getString();
    }
    else
    {
        // No arguments passed, get the map file name to load
        auto fileInfo = MapFileManager::getMapFileSelection(true, _("Select Map File to merge"), filetype::TYPE_MAP);

        if (fileInfo.fullPath.empty())
        {
            return; // operation cancelled
        }

        sourceCandidate = fileInfo.fullPath;
    }

    if (!os::fileOrDirExists(sourceCandidate))
    {
        throw cmd::ExecutionFailure(fmt::format(_("File doesn't exist: {0}"), sourceCandidate));
    }

    // Do we have a second argument (base map)
    if (args.size() > 1)
    {
        baseCandidate = args[1].getString();

        if (!os::fileOrDirExists(baseCandidate))
        {
            throw cmd::ExecutionFailure(fmt::format(_("File doesn't exist: {0}"), baseCandidate));
        }
    }

    if (!baseCandidate.empty())
    {
        startMergeOperation(sourceCandidate, baseCandidate);
    }
    else
    {
        startMergeOperation(sourceCandidate);
    }
}

void Map::abortMergeOperationCmd(const cmd::ArgumentList& args)
{
    abortMergeOperation();
}

void Map::finishMergeOperationCmd(const cmd::ArgumentList& args)
{
    finishMergeOperation();
}

void Map::emitMapEvent(MapEvent ev)
{
    try
    {
        signal_mapEvent().emit(ev);
    }
    catch (std::runtime_error & ex)
    {
        radiant::NotificationMessage::SendError(fmt::format(_("Failure running map event {0}:\n{1}"), ev, ex.what()));
    }
}

// RegisterableModule implementation
const std::string& Map::getName() const
{
    static std::string _name(MODULE_MAP);
    return _name;
}

const StringSet& Map::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_GAMEMANAGER,
        MODULE_SCENEGRAPH,
        MODULE_MAPINFOFILEMANAGER,
        MODULE_FILETYPES,
        MODULE_MAPRESOURCEMANAGER,
        MODULE_COMMANDSYSTEM
    };

    return _dependencies;
}

void Map::initialiseModule(const IApplicationContext& ctx)
{
    // Register for the startup event
	_mapPositionManager.reset(new MapPositionManager);

    GlobalSceneGraph().addSceneObserver(this);

    // Add the Map-related commands to the EventManager
    registerCommands();

    _scaledModelExporter.initialise();
    _modelScalePreserver.reset(new ModelScalePreserver);

    // Construct point trace and connect it to map signals
    _pointTrace.reset(new PointFile());
    signal_mapEvent().connect([this](IMap::MapEvent e)
                              { _pointTrace->onMapEvent(e); });

	MapFileManager::registerFileTypes();

    // Register an info file module to save the map property bag
    GlobalMapInfoFileManager().registerInfoFileModule(
        std::make_shared<MapPropertyInfoFileModule>()
    );

    // Free the map right before all modules are shut down
    module::GlobalModuleRegistry().signal_modulesUninitialising().connect(
        sigc::mem_fun(this, &Map::freeMap)
    );

    _shutdownListener = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::ApplicationShutdownRequest,
        radiant::TypeListener<radiant::ApplicationShutdownRequest>(
            sigc::mem_fun(this, &Map::handleShutdownRequest)));
}

void Map::shutdownModule()
{
    _undoEventListener.disconnect();

    abortMergeOperation();

    GlobalRadiantCore().getMessageBus().removeListener(_shutdownListener);

    _scaledModelExporter.shutdown();

	GlobalSceneGraph().removeSceneObserver(this);

    _modelScalePreserver.reset();
	_mapPositionManager.reset();
}

void Map::handleShutdownRequest(radiant::ApplicationShutdownRequest& request)
{
    if (!askForSave(_("Exit DarkRadiant")))
    {
        request.deny();
    }

    if (!request.isDenied())
    {
        abortMergeOperation();
    }
}

} // namespace map
