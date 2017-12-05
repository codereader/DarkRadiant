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
#include "iaasfile.h"
#include "igame.h"

#include "registry/registry.h"
#include "stream/TextFileInputStream.h"
#include "entitylib.h"
#include "gamelib.h"
#include "os/path.h"
#include "wxutil/IConv.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/ScopeTimer.h"

#include "brush/BrushModule.h"
#include "xyview/GlobalXYWnd.h"
#include "camera/GlobalCamera.h"
#include "scene/BasicRootNode.h"
#include "map/MapFileManager.h"
#include "map/MapPositionManager.h"
#include "map/StartupMapLoader.h"
#include "map/RootNode.h"
#include "map/MapResource.h"
#include "map/algorithm/Merge.h"
#include "map/algorithm/Export.h"
#include "map/algorithm/Traverse.h"
#include "map/algorithm/MapExporter.h"
#include "model/ModelExporter.h"
#include "map/algorithm/Skins.h"
#include "ui/mru/MRU.h"
#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/prefabselector/PrefabSelector.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Group.h"
#include "selection/algorithm/Transformation.h"
#include "modulesystem/ModuleRegistry.h"
#include "modulesystem/StaticModule.h"
#include "RenderableAasFile.h"

#include <fmt/format.h>
#include "algorithm/ChildPrimitives.h"

namespace map 
{

    namespace 
	{
        const char* const MAP_UNNAMED_STRING = N_("unnamed.map");

        const char* const GKEY_LAST_CAM_POSITION = "/mapFormat/lastCameraPositionKey";
        const char* const GKEY_LAST_CAM_ANGLE = "/mapFormat/lastCameraAngleKey";
        const char* const GKEY_PLAYER_START_ECLASS = "/mapFormat/playerStartPoint";
        const char* const GKEY_PLAYER_HEIGHT = "/defaults/playerHeight";
    }

Map::Map() :
    _lastCopyMapName(""),
    _saveInProgress(false)
{
	_mapSaveTimer.Pause();
}

void Map::loadMapResourceFromPath(const std::string& path)
{
	// Map loading started
	signal_mapEvent().emit(MapLoading);

	_resource = GlobalMapResourceManager().loadFromPath(_mapName);

    if (!_resource)
    {
        return;
    }

    if (isUnnamed() || !_resource->load())
    {
        // Map is unnamed or load failed, reset map resource node to empty
        _resource->setNode(std::make_shared<RootNode>(""));

        _resource->getNode()->getUndoChangeTracker().save();
        
        // Rename the map to "unnamed" in any case to avoid overwriting the failed map
        setMapName(_(MAP_UNNAMED_STRING));
    }

    // Take the new node and insert it as map root
    GlobalSceneGraph().setRoot(_resource->getNode());

	// Traverse the scenegraph and find the worldspawn
	findWorldspawn();

    // Associate the Scenegaph with the global RenderSystem
    // This usually takes a while since all editor textures are loaded - display a dialog to inform the user
    {
        ui::ScreenUpdateBlocker blocker(_("Processing..."), _("Loading textures..."), true); // force display

        GlobalSceneGraph().root()->setRenderSystem(std::dynamic_pointer_cast<RenderSystem>(
            module::GlobalModuleRegistry().getModule(MODULE_RENDERSYSTEM)));
    }

    // Map loading finished, emit the signal
    signal_mapEvent().emit(MapLoaded);
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
        return std::dynamic_pointer_cast<scene::IMapRootNode>(_resource->getNode());
    }

    return scene::IMapRootNodePtr();
}

MapFormatPtr Map::getFormatForFile(const std::string& filename)
{
    // Look up the module name which loads the given extension
    std::string gameType = GlobalGameManager().currentGame()->getKeyValue("type");

    MapFormatPtr mapFormat = GlobalMapFormatManager().getMapFormatForGameType(
        gameType, os::getExtension(filename));

    ASSERT_MESSAGE(mapFormat != NULL, "map format not found for file " + filename);

    return mapFormat;
}

MapFormatPtr Map::getFormat()
{
    return getFormatForFile(_mapName);
}

// free all map elements, reinitialize the structures that depend on them
void Map::freeMap() 
{
	// Fire the map unloading event, 
	// This will de-select stuff, clear the pointfile, etc.
	signal_mapEvent().emit(MapUnloading);

	setWorldspawn(scene::INodePtr());

	GlobalSceneGraph().setRoot(scene::IMapRootNodePtr());

	signal_mapEvent().emit(MapUnloaded);

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
void Map::focusViews(const Vector3& point, const Vector3& angles) {
    // Set the camera and the views to the given point
    GlobalCamera().focusCamera(point, angles);
    GlobalXYWnd().setOrigin(point);
}

void Map::removeCameraPosition() {
    const std::string keyLastCamPos = game::current::getValue<std::string>(GKEY_LAST_CAM_POSITION);
    const std::string keyLastCamAngle = game::current::getValue<std::string>(GKEY_LAST_CAM_ANGLE);

    if (_worldSpawnNode != NULL) {
        // Retrieve the entity from the worldspawn node
        Entity* worldspawn = Node_getEntity(_worldSpawnNode);
        assert(worldspawn != NULL); // This must succeed

        worldspawn->setKeyValue(keyLastCamPos, "");
        worldspawn->setKeyValue(keyLastCamAngle, "");
    }
}

/* greebo: Saves the current camera position/angles to worldspawn
 */
void Map::saveCameraPosition()
{
    const std::string keyLastCamPos = game::current::getValue<std::string>(GKEY_LAST_CAM_POSITION);
    const std::string keyLastCamAngle = game::current::getValue<std::string>(GKEY_LAST_CAM_ANGLE);

    if (_worldSpawnNode != NULL) {
        // Retrieve the entity from the worldspawn node
        Entity* worldspawn = Node_getEntity(_worldSpawnNode);
        assert(worldspawn != NULL); // This must succeed

        ui::CamWndPtr camWnd = GlobalCamera().getActiveCamWnd();
        if (camWnd == NULL) return;

        worldspawn->setKeyValue(keyLastCamPos,
                                string::to_string(camWnd->getCameraOrigin()));
        worldspawn->setKeyValue(keyLastCamAngle,
                                string::to_string(camWnd->getCameraAngles()));
    }
}

/** Find the start position in the map and focus the viewport on it.
 */
void Map::gotoStartPosition()
{
    const std::string keyLastCamPos = game::current::getValue<std::string>(GKEY_LAST_CAM_POSITION);
    const std::string keyLastCamAngle = game::current::getValue<std::string>(GKEY_LAST_CAM_ANGLE);
    const std::string eClassPlayerStart = game::current::getValue<std::string>(GKEY_PLAYER_START_ECLASS);

    Vector3 angles(0,0,0);
    Vector3 origin(0,0,0);

    if (_worldSpawnNode != NULL) {
        // Retrieve the entity from the worldspawn node
        Entity* worldspawn = Node_getEntity(_worldSpawnNode);
        assert(worldspawn != NULL); // This must succeed

        // Try to find a saved "last camera position"
        const std::string savedOrigin = worldspawn->getKeyValue(keyLastCamPos);

        if (savedOrigin != "")
        {
            // Construct the vector out of the std::string
            origin = string::convert<Vector3>(savedOrigin);

            angles = string::convert<Vector3>(
                worldspawn->getKeyValue(keyLastCamAngle)
            );

            // Focus the view with the default angle
            focusViews(origin, angles);

            // Remove the saved entity key value so it doesn't appear during map edit
            removeCameraPosition();

            return;
        }
        else
        {
            // Get the player start entity
            Entity* playerStart = Scene_FindEntityByClass(eClassPlayerStart);

            if (playerStart != NULL)
            {
                // Get the entity origin
                origin = string::convert<Vector3>(
                    playerStart->getKeyValue("origin")
                );

                // angua: move the camera upwards a bit
				origin.z() += game::current::getValue<float>(GKEY_PLAYER_HEIGHT);

                // Check for an angle key, and use it if present
                angles[ui::CAMERA_YAW] = string::convert<float>(playerStart->getKeyValue("angle"), 0);
            }
        }
    }

    // Focus the view with the given parameters
    focusViews(origin, angles);
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
	scene::INodePtr worldspawn(GlobalEntityCreator().createEntity(
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

    // Move the view to a start position
    gotoStartPosition();

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
    ui::ScreenUpdateBlocker blocker(_("Processing..."), "");

	blocker.setMessage(_("Preprocessing"));

	try
	{
		signal_mapEvent().emit(IMap::MapSaving);
	}
	catch (std::runtime_error& ex)
	{
		wxutil::Messagebox::ShowError(
			fmt::format(_("Failure running map pre-save event:\n{0}"), ex.what()));
	}

    // Store the camview position into worldspawn
    saveCameraPosition();

    wxutil::ScopeTimer timer("map save");

	blocker.setMessage(_("Saving Map"));

    // Save the actual map resource
    bool success = _resource->save(mapFormat);

	signal_mapEvent().emit(IMap::MapSaved);

    // Remove the saved camera position
    removeCameraPosition();

    if (success)
    {
        // Clear the modified flag
        setModified(false);
    }

    _saveInProgress = false;

    // Redraw the views, sometimes the backbuffer containing 
    // the previous frame will remain visible
    GlobalMainFrame().updateAllWindows();

    return success;
}

void Map::createNew() {
    setMapName(_(MAP_UNNAMED_STRING));

	loadMapResourceFromPath(_mapName);

    SceneChangeNotify();

    setModified(false);

    focusViews(Vector3(0,0,0), Vector3(0,0,0));
}

bool Map::import(const std::string& filename)
{
    ui::ScreenUpdateBlocker blocker(_("Importing..."), filename);

    bool success = false;

    {
        IMapResourcePtr resource = GlobalMapResourceManager().loadFromPath(filename);

        if (resource->load())
        {
            // load() returned TRUE, this means that the resource node
            // is not the NULL node
            scene::INodePtr otherRoot = resource->getNode();

            // Adjust all new names to fit into the existing map namespace,
            // this routine will be changing a lot of names in the importNamespace
            INamespacePtr nspace = getRoot()->getNamespace();

            if (nspace)
            {
                // Prepare our namespace for import
                nspace->ensureNoConflicts(otherRoot);

                // Now add the imported names to the local namespace
                nspace->connect(otherRoot);
            }

            MergeMap(otherRoot);
            success = true;
        }
    }

    SceneChangeNotify();

    return success;
}

bool Map::saveDirect(const std::string& filename, const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return false; // safeguard

    // Disable screen updates for the scope of this function
    ui::ScreenUpdateBlocker blocker(_("Processing..."), os::getFilename(filename));

    _saveInProgress = true;

	MapFormatPtr format = mapFormat;

	if (!mapFormat)
	{
		format = getFormatForFile(filename);
	}

    bool result = MapResource::saveFile(
        *format,
        GlobalSceneGraph().root(),
        map::traverse, // TraversalFunc
        filename
    );

    _saveInProgress = false;

    return result;
}

bool Map::saveSelected(const std::string& filename, const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return false; // safeguard

    // Disable screen updates for the scope of this function
    ui::ScreenUpdateBlocker blocker(_("Processing..."), os::getFilename(filename));

    _saveInProgress = true;

	MapFormatPtr format = mapFormat;

	if (!format)
	{
		format = getFormatForFile(filename);
	}

    bool success = MapResource::saveFile(
        *format,
        GlobalSceneGraph().root(),
        map::traverseSelected, // TraversalFunc
        filename
    );

    _saveInProgress = false;

    return success;
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
    ui::IDialogPtr msgBox = GlobalDialogManager().createMessageBox(
        title,
        getSaveConfirmationText(),
        ui::IDialog::MESSAGE_SAVECONFIRMATION
    );

    ui::IDialog::Result result = msgBox->run();

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

bool Map::saveCopyAs()
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
		return saveDirect(fileInfo.fullPath, fileInfo.mapFormat);
    }

    // Not executed, return false
    return false;
}

void Map::loadPrefabAt(const Vector3& targetCoords)
{
    /*MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(true, _("Load Prefab"), filetype::TYPE_PREFAB);*/

	ui::PrefabSelector::Result result = ui::PrefabSelector::ChoosePrefab();

	if (!result.prefabPath.empty())
	{
        UndoableCommand undo("loadPrefabAt");

        // Deselect everything
        GlobalSelectionSystem().setSelectedAll(false);

        // Now import the prefab (imported items get selected)
		import(result.prefabPath);

        // Switch texture lock on
        bool prevTexLockState = GlobalBrush().textureLockEnabled();
        GlobalBrush().setTextureLock(true);

        // Translate the selection to the given point
        selection::algorithm::translateSelected(targetCoords);

        // Revert to previous state
        GlobalBrush().setTextureLock(prevTexLockState);

		// Check whether we should group the prefab parts
		if (result.insertAsGroup && GlobalSelectionSystem().countSelected() > 1)
		{
			try
			{
				selection::algorithm::groupSelected();
			}
			catch (selection::algorithm::CommandNotAvailableException& ex)
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
    GlobalCommandSystem().addCommand("OpenMap", Map::openMap);
    GlobalCommandSystem().addCommand("ImportMap", Map::importMap);
    GlobalCommandSystem().addCommand("LoadPrefab", Map::loadPrefab);
    GlobalCommandSystem().addCommand("SaveSelectedAsPrefab", Map::saveSelectedAsPrefab);
    GlobalCommandSystem().addCommand("SaveMap", Map::saveMap);
    GlobalCommandSystem().addCommand("SaveMapAs", Map::saveMapAs);
    GlobalCommandSystem().addCommand("SaveMapCopyAs", Map::saveMapCopyAs);
    GlobalCommandSystem().addCommand("SaveSelected", Map::exportMap);
	GlobalCommandSystem().addCommand("ReloadSkins", map::algorithm::reloadSkins);
	GlobalCommandSystem().addCommand("ExportSelectedAsModel", map::algorithm::exportSelectedAsModelCmd,
		cmd::Signature(cmd::ARGTYPE_STRING, cmd::ARGTYPE_STRING, 
					   cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL, 
					   cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL, 
					   cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL));

    GlobalEventManager().addCommand("NewMap", "NewMap");
    GlobalEventManager().addCommand("OpenMap", "OpenMap");
    GlobalEventManager().addCommand("ImportMap", "ImportMap");
    GlobalEventManager().addCommand("LoadPrefab", "LoadPrefab");
    GlobalEventManager().addCommand("SaveSelectedAsPrefab", "SaveSelectedAsPrefab");
    GlobalEventManager().addCommand("SaveMap", "SaveMap");
    GlobalEventManager().addCommand("SaveMapAs", "SaveMapAs");
    GlobalEventManager().addCommand("SaveMapCopyAs", "SaveMapCopyAs");
    GlobalEventManager().addCommand("SaveSelected", "SaveSelected");
	GlobalEventManager().addCommand("ReloadSkins", "ReloadSkins");
	GlobalEventManager().addCommand("ExportSelectedAsModel", "ExportSelectedAsModel");
}

// Static command targets
void Map::newMap(const cmd::ArgumentList& args)
{
    if (GlobalMap().askForSave(_("New Map")))
	{
        GlobalMap().freeMap();
        GlobalMap().createNew();
    }
}

void Map::openMap(const cmd::ArgumentList& args)
{
    if (!GlobalMap().askForSave(_("Open Map")))
        return;

    // Get the map file name to load
    MapFileSelection fileInfo = MapFileManager::getMapFileSelection(true, _("Open map"), filetype::TYPE_MAP);

    if (!fileInfo.fullPath.empty())
	{
        GlobalMRU().insert(fileInfo.fullPath);

        GlobalMap().freeMap();
        GlobalMap().load(fileInfo.fullPath);
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
    MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(false, _("Export selection"), filetype::TYPE_MAP);

	if (!fileInfo.fullPath.empty())
	{
		GlobalMap().saveSelected(fileInfo.fullPath, fileInfo.mapFormat);
    }
}

void Map::loadPrefab(const cmd::ArgumentList& args) {
    GlobalMap().loadPrefabAt(Vector3(0,0,0));
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

void Map::importSelected(std::istream& in)
{
    scene::INodePtr root = std::make_shared<scene::BasicRootNode>();

    // Instantiate the default import filter
    class MapImportFilter :
        public IMapImportFilter
    {
    private:
        scene::INodePtr _root;
    public:
        MapImportFilter(const scene::INodePtr& root) :
            _root(root)
        {}

        bool addEntity(const scene::INodePtr& entityNode)
        {
            _root->addChildNode(entityNode);
            return true;
        }

        bool addPrimitiveToEntity(const scene::INodePtr& primitive, const scene::INodePtr& entity)
        {
            if (Node_getEntity(entity)->isContainer())
            {
                entity->addChildNode(primitive);
                return true;
            }
            else
            {
                return false;
            }
        }
    } importFilter(root);

    MapFormatPtr format = getFormat();

    IMapReaderPtr reader = format->getMapReader(importFilter);

    try
    {
        // Start parsing
        reader->readFromStream(in);

        // Prepare child primitives
        addOriginToChildPrimitives(root);

        // Adjust all new names to fit into the existing map namespace,
        // this routine will be changing a lot of names in the importNamespace
        INamespacePtr nspace = getRoot()->getNamespace();
        if (nspace)
        {
            // Prepare all names, but do not import them into the namesace. This
            // will happen during the MergeMap call.
            nspace->ensureNoConflicts(root);
        }

        MergeMap(root);
    }
    catch (IMapReader::FailureException& e)
    {
        wxutil::Messagebox::ShowError(
            fmt::format(_("Failure reading map from clipboard:\n{0}"), e.what()));

        // Clear out the root node, otherwise we end up with half a map
        scene::NodeRemover remover;
        root->traverseChildren(remover);
    }
}

void Map::exportSelected(std::ostream& out)
{
    MapFormatPtr format = getFormat();

    IMapWriterPtr writer = format->getMapWriter();

    // Create our main MapExporter walker for traversal
    MapExporter exporter(*writer, GlobalSceneGraph().root(), out);

    // Pass the traverseSelected function and start writing selected nodes
    exporter.exportMap(GlobalSceneGraph().root(), traverseSelected);
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
        _dependencies.insert(MODULE_RADIANT);
		_dependencies.insert(MODULE_GAMEMANAGER);
		_dependencies.insert(MODULE_SCENEGRAPH);
		_dependencies.insert(MODULE_FILETYPES);
    }

    return _dependencies;
}

void Map::initialiseModule(const ApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    // Register for the startup event
    _startupMapLoader.reset(new StartupMapLoader);
	_mapPositionManager.reset(new MapPositionManager);

    GlobalSceneGraph().addSceneObserver(this);

    // Add the Map-related commands to the EventManager
    registerCommands();

	_scaledModelExporter.initialise();

	MapFileManager::registerFileTypes();
}

void Map::shutdownModule()
{
	_scaledModelExporter.shutdown();

	GlobalSceneGraph().removeSceneObserver(this);

	_startupMapLoader.reset();
	_mapPositionManager.reset();
}

// Creates the static module instance
module::StaticModule<Map> staticMapModule;

} // namespace map

// Accessor method containing the singleton Map instance
map::Map& GlobalMap() 
{
    return *map::staticMapModule.getModule();
}
