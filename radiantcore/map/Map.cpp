#include "Map.h"

#include "i18n.h"
#include <ostream>
#include <fstream>
#include "itextstream.h"
#include "iscenegraph.h"
#include "idialogmanager.h"
#include "ieventmanager.h"
#include "imodel.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "iselectiongroup.h"
#include "ifilter.h"
#include "icounter.h"
#include "iradiant.h"
#include "imainframe.h"
#include "imapresource.h"
#include "imapinfofile.h"
#include "iaasfile.h"
#include "igame.h"
#include "imru.h"
#include "imapformat.h"

#include "registry/registry.h"
#include "stream/TextFileInputStream.h"
#include "entitylib.h"
#include "gamelib.h"
#include "os/path.h"
#include "os/file.h"
#include "wxutil/IConv.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/ScopeTimer.h"

#include "brush/BrushModule.h"
#include "scene/BasicRootNode.h"
#include "map/MapFileManager.h"
#include "map/MapPositionManager.h"
#include "map/MapResource.h"
#include "map/algorithm/Import.h"
#include "map/algorithm/Export.h"
#include "scene/Traverse.h"
#include "map/algorithm/MapExporter.h"
#include "model/export/ModelExporter.h"
#include "model/export/ModelScalePreserver.h"
#include "map/algorithm/Skins.h"
#include "messages/ScopedLongRunningOperation.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Group.h"
#include "selection/algorithm/Transformation.h"
#include "module/StaticModule.h"
#include "command/ExecutionNotPossible.h"
#include "MapPropertyInfoFileModule.h"

#include <fmt/format.h>
#include "scene/ChildPrimitives.h"

namespace map 
{

namespace 
{
    const char* const MAP_UNNAMED_STRING = N_("unnamed.map");
}

Map::Map() :
    _lastCopyMapName(""),
    _saveInProgress(false),
    _shutdownListener(0)
{
	_mapSaveTimer.Pause();
}

void Map::clearMapResource()
{
    // Map is unnamed or load failed, reset map resource node to empty
    _resource->clear();

    _resource->getRootNode()->getUndoChangeTracker().save();

    // Rename the map to "unnamed" in any case to avoid overwriting the failed map
    setMapName(_(MAP_UNNAMED_STRING));
}

void Map::loadMapResourceFromPath(const std::string& path)
{
	// Map loading started
	emitMapEvent(MapLoading);

	_resource = GlobalMapResourceManager().loadFromPath(_mapName);

    if (!_resource)
    {
        return;
    }

    try
    {
        if (isUnnamed() || !_resource->load())
        {
            clearMapResource();
        }
    }
    catch (const IMapResource::OperationException & ex)
    {
        wxutil::Messagebox::ShowError(ex.what());
        clearMapResource();
    }

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

    // Map loading finished, emit the signal
    emitMapEvent(MapLoaded);
}

void Map::updateTitle()
{
    std::string title = _mapName;

    if (m_modified)
	{
        title += " *";
    }

	if (GlobalMainFrame().getWxTopLevelWindow())
    {
		GlobalMainFrame().getWxTopLevelWindow()->SetTitle(title);
    }
}

void Map::setMapName(const std::string& newName) {
    // Store the name into the member
    _mapName = newName;

    // Update the map resource's root node, if there is one
    if (_resource)
	{
        _resource->rename(newName);
    }

    // Update the title of the main window
    updateTitle();
}

std::string Map::getMapName() const {
    return _mapName;
}

bool Map::isUnnamed() const {
    return _mapName == _(MAP_UNNAMED_STRING);
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

// free all map elements, reinitialize the structures that depend on them
void Map::freeMap() 
{
	// Fire the map unloading event, 
	// This will de-select stuff, clear the pointfile, etc.
    emitMapEvent(MapUnloading);

	setWorldspawn(scene::INodePtr());

	GlobalSceneGraph().setRoot(scene::IMapRootNodePtr());

    emitMapEvent(MapUnloaded);

    // Reset the resource pointer
    _resource.reset();
}

bool Map::isModified() const {
    return m_modified;
}

// Set the modified flag
void Map::setModified(bool modifiedFlag)
{
    m_modified = modifiedFlag;
    updateTitle();

    // Reset the map save timer
    _mapSaveTimer.Start();
}

// move the view to a certain position
void Map::focusViews(const Vector3& point, const Vector3& angles)
{
    // Set the camera and the views to the given point
    GlobalCameraView().focusCamera(point, angles);
    GlobalXYWndManager().setOrigin(point);
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

void Map::load(const std::string& filename) {
    rMessage() << "Loading map from " << filename << "\n";

    setMapName(filename);

    {
        wxutil::ScopeTimer timer("map load");

		loadMapResourceFromPath(_mapName);
    }

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

bool Map::save(const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return false; // safeguard

    _saveInProgress = true;

    // Disable screen updates for the scope of this function
    radiant::ScopedLongRunningOperation blocker(_("Saving Map..."));

    emitMapEvent(MapSaving);

    wxutil::ScopeTimer timer("map save");

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
        wxutil::Messagebox::ShowError(ex.what());
    }

    emitMapEvent(MapSaved);

    _saveInProgress = false;

    // Redraw the views, sometimes the backbuffer containing 
    // the previous frame will remain visible
    GlobalMainFrame().updateAllWindows();

    return success;
}

void Map::createNewMap()
{
    setMapName(_(MAP_UNNAMED_STRING));

	loadMapResourceFromPath(_mapName);

    SceneChangeNotify();

    setModified(false);

    focusViews(Vector3(0,0,0), Vector3(0,0,0));
}

bool Map::import(const std::string& filename)
{
    radiant::ScopedLongRunningOperation blocker(_("Importing..."));

    bool success = false;

    IMapResourcePtr resource = GlobalMapResourceManager().loadFromPath(filename);

    try
    {
        if (resource->load())
        {
            // load() returned TRUE, this means that the resource node
            // is not the NULL node
            const auto& otherRoot = resource->getRootNode();

            // Adjust all new names to fit into the existing map namespace
            algorithm::prepareNamesForImport(getRoot(), otherRoot);

            algorithm::mergeMap(otherRoot);
            success = true;
        }

        SceneChangeNotify();
    }
    catch (const IMapResource::OperationException& ex)
    {
        wxutil::Messagebox::ShowError(ex.what());
    }

    return success;
}

void Map::saveDirect(const std::string& filename, const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return; // safeguard

    // Disable screen updates for the scope of this function
    radiant::ScopedLongRunningOperation blocker(_("Saving..."));

    _saveInProgress = true;

	MapFormatPtr format = mapFormat;

	if (!mapFormat)
	{
		format = GlobalMapFormatManager().getMapFormatForFilename(filename);
	}

    try
    {
        MapResource::saveFile(*format, GlobalSceneGraph().root(), scene::traverse, filename);
    }
    catch (const IMapResource::OperationException& ex)
    {
        wxutil::Messagebox::ShowError(ex.what());
    }

    _saveInProgress = false;
}

void Map::saveSelected(const std::string& filename, const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return; // safeguard

    // Disable screen updates for the scope of this function
    radiant::ScopedLongRunningOperation blocker(_("Saving..."));

    _saveInProgress = true;

	MapFormatPtr format = mapFormat;

	if (!format)
	{
		format = GlobalMapFormatManager().getMapFormatForFilename(filename);
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
        wxutil::Messagebox::ShowError(ex.what());
    }

    _saveInProgress = false;
}

std::string Map::getSaveConfirmationText() const
{
    std::string primaryText = fmt::format(_("Save changes to map \"{0}\"\nbefore closing?"), _mapName);

    // Display "x seconds" or "x minutes"
    int seconds = static_cast<int>(_mapSaveTimer.Time() / 1000);
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
    auto msgBox = GlobalDialogManager().createMessageBox(
        title,
        getSaveConfirmationText(),
        ui::IDialog::MESSAGE_SAVECONFIRMATION
    );

    auto result = msgBox->run();

    if (result == ui::IDialog::RESULT_CANCELLED)
    {
        return false;
    }

    if (result == ui::IDialog::RESULT_YES)
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

    // Default behaviour: allow the save
    return true;
}

bool Map::saveAs()
{
    if (_saveInProgress) return false; // safeguard

    MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(false, _("Save Map"), filetype::TYPE_MAP, getMapName());

	if (!fileInfo.fullPath.empty())
	{
        // Remember the old name, we might need to revert
        std::string oldFilename = _mapName;

        // Rename the file and try to save
        rename(fileInfo.fullPath);

        // Try to save the file, this might fail
		bool success = save(fileInfo.mapFormat);

        if (success)
		{
            GlobalMRU().insert(fileInfo.fullPath);
        }
        else if (!success)
		{
            // Revert the name change if the file could not be saved
            rename(oldFilename);
        }

        return success;
    }
    else
	{
        // Invalid filename entered, return false
        return false;
    }
}

void Map::saveCopyAs()
{
    // Let's see if we can remember a
    if (_lastCopyMapName.empty()) {
        _lastCopyMapName = getMapName();
    }

	MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(false, _("Save Copy As..."), filetype::TYPE_MAP, _lastCopyMapName);

	if (!fileInfo.fullPath.empty())
	{
        // Remember the last name
        _lastCopyMapName = fileInfo.fullPath;

        // Return the result of the actual save method
		saveDirect(fileInfo.fullPath, fileInfo.mapFormat);
    }
}

void Map::loadPrefabAt(const cmd::ArgumentList& args)
{
    if (args.size() < 2 || args.size() > 3)
    {
        rWarning() << "Usage: " << LOAD_PREFAB_AT_CMD << 
            " <prefabPath:String> <targetCoords:Vector3> [insertAsGroup:0|1]" << std::endl;
        return;
    }

    auto prefabPath = args[0].getString();
    auto targetCoords = args[1].getVector3();
    auto insertAsGroup = args.size() > 2 ? args[2].getBoolean() : false;

	if (!prefabPath.empty())
	{
        UndoableCommand undo("loadPrefabAt");

        // Deselect everything
        GlobalSelectionSystem().setSelectedAll(false);

        // Now import the prefab (imported items get selected)
		import(prefabPath);

        // Switch texture lock on
        bool prevTexLockState = GlobalBrush().textureLockEnabled();
        GlobalBrush().setTextureLock(true);

        // Translate the selection to the given point
        selection::algorithm::translateSelected(targetCoords);

        // Revert to previous state
        GlobalBrush().setTextureLock(prevTexLockState);

		// Check whether we should group the prefab parts
		if (insertAsGroup && GlobalSelectionSystem().countSelected() > 1)
		{
			try
			{
				selection::algorithm::groupSelected();
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
    GlobalMap().saveCopyAs();
}

void Map::registerCommands()
{
    GlobalCommandSystem().addCommand("NewMap", Map::newMap);
    GlobalCommandSystem().addCommand("OpenMap", Map::openMap, { cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL });
    GlobalCommandSystem().addCommand("ImportMap", Map::importMap);
    GlobalCommandSystem().addCommand(LOAD_PREFAB_AT_CMD, std::bind(&Map::loadPrefabAt, this, std::placeholders::_1), 
        { cmd::ARGTYPE_STRING, cmd::ARGTYPE_VECTOR3, cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL });
    GlobalCommandSystem().addCommand("SaveSelectedAsPrefab", Map::saveSelectedAsPrefab);
    GlobalCommandSystem().addCommand("SaveMap", Map::saveMap);
    GlobalCommandSystem().addCommand("SaveMapAs", Map::saveMapAs);
    GlobalCommandSystem().addCommand("SaveMapCopyAs", Map::saveMapCopyAs);
    GlobalCommandSystem().addCommand("ExportMap", Map::exportMap);
    GlobalCommandSystem().addCommand("SaveSelected", Map::exportSelection);
	GlobalCommandSystem().addCommand("ReloadSkins", map::algorithm::reloadSkins);
	GlobalCommandSystem().addCommand("ExportSelectedAsModel", map::algorithm::exportSelectedAsModelCmd,
        { cmd::ARGTYPE_STRING, 
          cmd::ARGTYPE_STRING,
          cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL,
          cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL,
          cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL });
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

void Map::openMap(const cmd::ArgumentList& args)
{
    if (!GlobalMap().askForSave(_("Open Map"))) return;

    std::string candidate;

    if (!args.empty())
    {
        candidate = args[0].getString();
    }
    else
    {
        // No arguments passed, get the map file name to load
        MapFileSelection fileInfo = MapFileManager::getMapFileSelection(true, _("Open map"), filetype::TYPE_MAP);
        candidate = fileInfo.fullPath;
    }

    std::string mapToLoad;

    if (os::fileOrDirExists(candidate))
    {
        mapToLoad = candidate;
    }
    else if (!candidate.empty())
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
            throw cmd::ExecutionFailure(fmt::format(_("File doesn't exist: {0}"), fullMapPath.string()));
        }
    }

    if (!mapToLoad.empty())
	{
        GlobalMRU().insert(mapToLoad);

        GlobalMap().freeMap();
        GlobalMap().load(mapToLoad);
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

void Map::saveMap(const cmd::ArgumentList& args)
{
    if (GlobalMap().isUnnamed())
    {
        GlobalMap().saveAs();
    }
    // greebo: Always let the map be saved, regardless of the modified status.
    else /*if(GlobalMap().isModified())*/
    {
        GlobalMap().save();
    }
}

void Map::exportMap(const cmd::ArgumentList& args)
{
    auto fileInfo = MapFileManager::getMapFileSelection(false, _("Export Map"), filetype::TYPE_MAP_EXPORT);

	if (!fileInfo.fullPath.empty())
	{
        GlobalMap().emitMapEvent(MapSaving);

        MapResource::saveFile(*fileInfo.mapFormat,
            GlobalSceneGraph().root(),
            scene::traverse,
            fileInfo.fullPath);

        GlobalMap().emitMapEvent(MapSaved);
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

void Map::rename(const std::string& filename) {
    if (_mapName != filename) {
        setMapName(filename);
        SceneChangeNotify();
    }
    else {
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
    MapExporter exporter(*format, GlobalSceneGraph().root(), out);

    // Pass the traverseSelected function and start writing selected nodes
    exporter.exportMap(GlobalSceneGraph().root(), scene::traverseSelected);
}

void Map::emitMapEvent(MapEvent ev)
{
    try
    {
        signal_mapEvent().emit(ev);
    }
    catch (std::runtime_error & ex)
    {
        wxutil::Messagebox::ShowError(fmt::format(_("Failure running map event {0}:\n{1}"), ev, ex.what()));
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
    static StringSet _dependencies;

    if (_dependencies.empty())
	{
        _dependencies.insert(MODULE_RADIANT_APP);
		_dependencies.insert(MODULE_GAMEMANAGER);
		_dependencies.insert(MODULE_SCENEGRAPH);
		_dependencies.insert(MODULE_MAPINFOFILEMANAGER);
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_MAPRESOURCEMANAGER);
    }

    return _dependencies;
}

void Map::initialiseModule(const ApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    // Register for the startup event
	_mapPositionManager.reset(new MapPositionManager);

    GlobalSceneGraph().addSceneObserver(this);

    // Add the Map-related commands to the EventManager
    registerCommands();

    _scaledModelExporter.initialise();
    _modelScalePreserver.reset(new ModelScalePreserver);

	MapFileManager::registerFileTypes();

    // Register an info file module to save the map property bag
    GlobalMapInfoFileManager().registerInfoFileModule(
        std::make_shared<MapPropertyInfoFileModule>()
    );

    GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(this, &Map::onRadiantShutdown)
    );

    _shutdownListener = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::ApplicationShutdownRequest,
        radiant::TypeListener<radiant::ApplicationShutdownRequest>(
            sigc::mem_fun(this, &Map::handleShutdownRequest)));
}

void Map::shutdownModule()
{
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
}

void Map::onRadiantShutdown()
{
    freeMap();
}

} // namespace map
