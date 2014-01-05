#include "Map.h"

#include "i18n.h"
#include <ostream>
#include "itextstream.h"
#include "iscenegraph.h"
#include "idialogmanager.h"
#include "ieventmanager.h"
#include "iundo.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "ifilter.h"
#include "icounter.h"
#include "iradiant.h"
#include "imainframe.h"
#include "imapresource.h"
#include "iselectionset.h"

#include "registry/registry.h"
#include "stream/textfilestream.h"
#include "entitylib.h"
#include "gamelib.h"
#include "os/path.h"
#include "gtkutil/IConv.h"
#include "gtkutil/dialog/MessageBox.h"

#include "brush/BrushModule.h"
#include "xyview/GlobalXYWnd.h"
#include "camera/GlobalCamera.h"
#include "map/AutoSaver.h"
#include "map/BasicContainer.h"
#include "map/MapFileManager.h"
#include "map/MapPositionManager.h"
#include "map/PointFile.h"
#include "map/RegionManager.h"
#include "map/RootNode.h"
#include "map/MapResource.h"
#include "map/algorithm/Clone.h"
#include "map/algorithm/Merge.h"
#include "map/algorithm/Traverse.h"
#include "map/algorithm/MapExporter.h"
#include "map/algorithm/Skins.h"
#include "ui/mru/MRU.h"
#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/layers/LayerControlDialog.h"
#include "selection/algorithm/Primitives.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "modulesystem/ModuleRegistry.h"
#include "modulesystem/StaticModule.h"

#include "algorithm/ChildPrimitives.h"

namespace map {

    namespace {
        const char* const MAP_UNNAMED_STRING = N_("unnamed.map");

        const char* const GKEY_LAST_CAM_POSITION = "/mapFormat/lastCameraPositionKey";
        const char* const GKEY_LAST_CAM_ANGLE = "/mapFormat/lastCameraAngleKey";
        const char* const GKEY_PLAYER_START_ECLASS = "/mapFormat/playerStartPoint";
        const char* const GKEY_PLAYER_HEIGHT = "/defaults/playerHeight";

        // Traverse all entities and store the first worldspawn into the map
        class MapWorldspawnFinder :
            public scene::NodeVisitor
        {
        public:
            virtual bool pre(const scene::INodePtr& node) {
                if (node_is_worldspawn(node)) {
                    if (GlobalMap().getWorldspawn() == NULL) {
                        GlobalMap().setWorldspawn(node);
                    }
                }
                return false;
            }
        };

        class CollectAllWalker :
            public scene::NodeVisitor
        {
            scene::INodePtr _root;
            std::vector<scene::INodePtr>& _nodes;
        public:
            CollectAllWalker(scene::INodePtr root, std::vector<scene::INodePtr>& nodes) :
                _root(root),
                _nodes(nodes)
            {}

            ~CollectAllWalker() {
                for (std::vector<scene::INodePtr>::iterator i = _nodes.begin();
                     i != _nodes.end(); ++i)
                {
                    _root->removeChildNode(*i);
                }
            }

            virtual bool pre(const scene::INodePtr& node) {
                // Add this to the list
                _nodes.push_back(node);
                // Don't traverse deeper than first level
                return false;
            }
        };

        void Node_insertChildFirst(scene::INodePtr parent, scene::INodePtr child) {
            // Create a container to collect all the existing entities in the scenegraph
            std::vector<scene::INodePtr> nodes;

            // Collect all the child nodes of <parent> and move them into the container
            {
                CollectAllWalker visitor(parent, nodes);
                parent->traverseChildren(visitor);

                // the CollectAllWalker removes the nodes from the parent on destruction
            }

            // Now that the <parent> is empty, insert the worldspawn as first child
            parent->addChildNode(child);

            // Insert all the nodes again
            for (std::vector<scene::INodePtr>::iterator i = nodes.begin();
                 i != nodes.end();
                 ++i)
            {
                parent->addChildNode(*i);
            }
        }

        scene::INodePtr createWorldspawn()
        {
          scene::INodePtr worldspawn(GlobalEntityCreator().createEntity(GlobalEntityClassManager().findOrInsert("worldspawn", true)));
          Node_insertChildFirst(GlobalSceneGraph().root(), worldspawn);
          return worldspawn;
        }
    }

Map::Map() :
    _lastCopyMapName(""),
    m_valid(false),
    _saveInProgress(false)
{}

void Map::realiseResource() {
    if (m_resource != NULL) {
        m_resource->realise();
    }
}

void Map::unrealiseResource() {
    if (m_resource != NULL) {
        m_resource->unrealise();
    }
}

void Map::onResourceRealise() {
    if (m_resource == NULL) {
        return;
    }

    if (isUnnamed() || !m_resource->load()) {
        // Map is unnamed or load failed, reset map resource node to empty
        m_resource->setNode(NewMapRoot(""));
        MapFilePtr map = Node_getMapFile(m_resource->getNode());

        if (map != NULL) {
            map->save();
        }

        // Rename the map to "unnamed" in any case to avoid overwriting the failed map
        setMapName(_(MAP_UNNAMED_STRING));
    }

    // Take the new node and insert it as map root
    GlobalSceneGraph().setRoot(m_resource->getNode());

    // Associate the Scenegaph with the global RenderSystem
    // This usually takes a while since all editor textures are loaded - display a dialog to inform the user
    {
        ui::ScreenUpdateBlocker blocker(_("Processing..."), _("Loading textures..."), true); // force display

        GlobalSceneGraph().root()->setRenderSystem(boost::dynamic_pointer_cast<RenderSystem>(
            module::GlobalModuleRegistry().getModule(MODULE_RENDERSYSTEM)));
    }

    AutoSaver().clearChanges();

    setValid(true);
}

void Map::onResourceUnrealise() {
    if(m_resource != 0)
    {
        setValid(false);
      setWorldspawn(scene::INodePtr());

      GlobalUndoSystem().clear();
      GlobalSelectionSetManager().deleteAllSelectionSets();

      GlobalSceneGraph().setRoot(scene::INodePtr());
    }
}

sigc::signal<void> Map::signal_mapValidityChanged() const
{
    return _sigMapValidityChanged;
}

void Map::setValid(bool valid)
{
    m_valid = valid;
    _sigMapValidityChanged();
}

bool Map::isValid() const
{
    return m_valid;
}

void Map::updateTitle()
{
    std::string title = gtkutil::IConv::localeToUTF8(_mapName);

    if (m_modified) {
        title += " *";
    }

    if (GlobalMainFrame().getTopLevelWindow())
    {
        GlobalMainFrame().getTopLevelWindow()->set_title(title);
    }
}

void Map::setMapName(const std::string& newName) {
    // Store the name into the member
    _mapName = newName;

    // Update the map resource's root node, if there is one
    if (m_resource != NULL) {
        m_resource->rename(newName);
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

void Map::setWorldspawn(scene::INodePtr node) {
    m_world_node = node;
}

scene::INodePtr Map::getWorldspawn() {
    return m_world_node;
}

IMapRootNodePtr Map::getRoot() {
    if (m_resource != NULL) {
        // Try to cast the node onto a root node and return
        return boost::dynamic_pointer_cast<IMapRootNode>(m_resource->getNode());
    }

    return IMapRootNodePtr();
}

MapFormatPtr Map::getFormatForFile(const std::string& filename)
{
    // Look up the module name which loads the given extension
    std::string gameType = GlobalGameManager().currentGame()->getKeyValue("type");

    MapFormatPtr mapFormat = GlobalMapFormatManager().getMapFormatForGameType(
        gameType, path_get_extension(filename.c_str()));

    ASSERT_MESSAGE(mapFormat != NULL, "map format not found for file " + filename);

    return mapFormat;
}

MapFormatPtr Map::getFormat()
{
    return getFormatForFile(_mapName);
}

// free all map elements, reinitialize the structures that depend on them
void Map::freeMap() {
    map::PointFile::Instance().clear();

    GlobalSelectionSystem().setSelectedAll(false);
    GlobalSelectionSystem().setSelectedAllComponents(false);

    GlobalShaderClipboard().clear();
    GlobalRegion().clear();

    m_resource->removeObserver(*this);

    // Reset the resource pointer
    m_resource = IMapResourcePtr();

    GlobalLayerSystem().reset();
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
    _mapSaveTimer.reset();
    _mapSaveTimer.start();
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

    if (m_world_node != NULL) {
        // Retrieve the entity from the worldspawn node
        Entity* worldspawn = Node_getEntity(m_world_node);
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

    if (m_world_node != NULL) {
        // Retrieve the entity from the worldspawn node
        Entity* worldspawn = Node_getEntity(m_world_node);
        assert(worldspawn != NULL); // This must succeed

        CamWndPtr camWnd = GlobalCamera().getActiveCamWnd();
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

    if (m_world_node != NULL) {
        // Retrieve the entity from the worldspawn node
        Entity* worldspawn = Node_getEntity(m_world_node);
        assert(worldspawn != NULL); // This must succeed

        // Try to find a saved "last camera position"
        const std::string savedOrigin = worldspawn->getKeyValue(keyLastCamPos);

        if (savedOrigin != "")
        {
            // Construct the vector out of the std::string
            origin = string::convert<Vector3>(savedOrigin);

            Vector3 angles = string::convert<Vector3>(
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
                try {
                    angles[CAMERA_YAW] = boost::lexical_cast<float>(playerStart->getKeyValue("angle"));
                }
                catch (boost::bad_lexical_cast e) {
                    angles[CAMERA_YAW] = 0;
                }
            }
        }
    }

    // Focus the view with the given parameters
    focusViews(origin, angles);
}

scene::INodePtr Map::findWorldspawn() {
    // Clear the current worldspawn node
    setWorldspawn(scene::INodePtr());

    // Traverse the scenegraph and search for the worldspawn
    MapWorldspawnFinder visitor;
    GlobalSceneGraph().root()->traverseChildren(visitor);

    return getWorldspawn();
}

void Map::updateWorldspawn() {
    if (findWorldspawn() == NULL) {
        setWorldspawn(createWorldspawn());
    }
}

scene::INodePtr Map::findOrInsertWorldspawn() {
    updateWorldspawn();
    return getWorldspawn();
}

void Map::load(const std::string& filename) {
    rMessage() << "Loading map from " << filename << "\n";

    setMapName(filename);

    // Reset all layers before loading the file
    GlobalLayerSystem().reset();
    GlobalSelectionSystem().setSelectedAll(false);

    {
        ScopeTimer timer("map load");

        m_resource = GlobalMapResourceManager().capture(_mapName);
        // greebo: Add the observer, this usually triggers a onResourceRealise() call.
        m_resource->addObserver(*this);

        // Traverse the scenegraph and find the worldspawn
        MapWorldspawnFinder finder;
        GlobalSceneGraph().root()->traverseChildren(finder);
    }

    rMessage() << "--- LoadMapFile ---\n";
    rMessage() << _mapName << "\n";

    rMessage() << GlobalCounters().getCounter(counterBrushes).get() << " brushes\n";
    rMessage() << GlobalCounters().getCounter(counterPatches).get() << " patches\n";
    rMessage() << GlobalCounters().getCounter(counterEntities).get() << " entities\n";

    // Move the view to a start position
    gotoStartPosition();

    // Load the stored map positions from the worldspawn entity
    GlobalMapPosition().loadPositions();
    // Remove them, so that the user doesn't get bothered with them
    GlobalMapPosition().removePositions();

    // Disable the region to make sure
    GlobalRegion().disable();

    // Clear the shaderclipboard, the references are most probably invalid now
    GlobalShaderClipboard().clear();

    // Let the filtersystem update the filtered status of all instances
    GlobalFilterSystem().update();

    // Update the layer control dialog
    ui::LayerControlDialog::Instance().refresh();

    // Clear the modified flag
    setModified(false);
}

bool Map::save(const MapFormatPtr& mapFormat)
{
    if (_saveInProgress) return false; // safeguard

    _saveInProgress = true;

    // Disable screen updates for the scope of this function
    ui::ScreenUpdateBlocker blocker(_("Processing..."), _("Saving Map"));

    // Store the camview position into worldspawn
    saveCameraPosition();

    // Store the map positions into the worldspawn spawnargs
    GlobalMapPosition().savePositions();

    PointFile::Instance().clear();

    ScopeTimer timer("map save");

    // Save the actual map resource
    bool success = m_resource->save(mapFormat);

    // Remove the saved camera position
    removeCameraPosition();

    // Remove the map positions again after saving
    GlobalMapPosition().removePositions();

    if (success)
    {
        // Clear the modified flag
        setModified(false);
    }

    _saveInProgress = false;

    return success;
}

void Map::createNew() {
    setMapName(_(MAP_UNNAMED_STRING));

    m_resource = GlobalMapResourceManager().capture(_mapName);
    m_resource->addObserver(*this);

    SceneChangeNotify();

    setModified(false);

    focusViews(Vector3(0,0,0), Vector3(0,0,0));
}

bool Map::import(const std::string& filename)
{
    ui::ScreenUpdateBlocker blocker(_("Importing..."), filename);

    bool success = false;

    {
        IMapResourcePtr resource = GlobalMapResourceManager().capture(filename);

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
    ui::ScreenUpdateBlocker blocker(_("Processing..."), path_get_filename_start(filename.c_str()));

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
    ui::ScreenUpdateBlocker blocker(_("Processing..."), path_get_filename_start(filename.c_str()));

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

Glib::ustring Map::getSaveConfirmationText() const
{
    Glib::ustring primaryText = Glib::ustring::compose(
        _("Save changes to map \"%1\"\nbefore closing?"),
        _mapName
    );

    // Display "x seconds" or "x minutes"
    int seconds = static_cast<int>(_mapSaveTimer.elapsed());
    Glib::ustring timeString;
    if (seconds > 120)
    {
        timeString = Glib::ustring::compose(
            _("%1 minutes"), seconds / 60
        );
    }
    else
    {
        timeString = Glib::ustring::compose(
            _("%1 seconds"), seconds
        );
    }

    Glib::ustring secondaryText = Glib::ustring::compose(
        _("If you don't save, changes from the last %1\nwill be lost."),
        timeString
    );

    Glib::ustring confirmText = Glib::ustring::compose(
        "<span weight=\"bold\" size=\"larger\">%1</span>\n\n%2",
         primaryText,
         secondaryText
    );

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
        MapFileManager::getMapFileSelection(false, _("Save Map"), "map", getMapName());

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
        MapFileManager::getMapFileSelection(false, _("Save Copy As..."), "map", _lastCopyMapName);

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
    MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(true, _("Load Prefab"), "prefab");

	if (!fileInfo.fullPath.empty())
	{
        UndoableCommand undo("loadPrefabAt");

        // Deselect everything
        GlobalSelectionSystem().setSelectedAll(false);

        // Now import the prefab (imported items get selected)
        import(fileInfo.fullPath);

        // Switch texture lock on
        bool prevTexLockState = GlobalBrush().textureLockEnabled();
        GlobalBrush().setTextureLock(true);

        // Translate the selection to the given point
        GlobalSelectionSystem().translateSelected(targetCoords);

        // Revert to previous state
        GlobalBrush().setTextureLock(prevTexLockState);
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
}

// Static command targets
void Map::newMap(const cmd::ArgumentList& args) {
    if (GlobalMap().askForSave(_("New Map"))) {
        // Turn regioning off when starting a new map
        GlobalRegion().disable();

        GlobalMap().freeMap();
        GlobalMap().createNew();
    }
}

void Map::openMap(const cmd::ArgumentList& args)
{
    if (!GlobalMap().askForSave(_("Open Map")))
        return;

    // Get the map file name to load
    MapFileSelection fileInfo = MapFileManager::getMapFileSelection(true, _("Open map"), "map");

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
        MapFileManager::getMapFileSelection(true, _("Import map"), "map");

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
        MapFileManager::getMapFileSelection(false, _("Export selection"), "map");

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
        MapFileManager::getMapFileSelection(false, _("Save selected as Prefab"), "prefab");

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
        m_resource->save();
        setModified(false);
    }
}

void Map::importSelected(std::istream& in)
{
    BasicContainerPtr root(new BasicContainer);

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
        gtkutil::MessageBox::ShowError(
            (boost::format(_("Failure reading map from clipboard:\n%s")) % e.what()).str(),
            GlobalMainFrame().getTopLevelWindow());

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
const std::string& Map::getName() const {
    static std::string _name(MODULE_MAP);
    return _name;
}

const StringSet& Map::getDependencies() const {
    static StringSet _dependencies;

    if (_dependencies.empty()) {
        _dependencies.insert(MODULE_RADIANT);
    }

    return _dependencies;
}

void Map::initialiseModule(const ApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    // Register for the startup event
    _startupMapLoader = StartupMapLoaderPtr(new StartupMapLoader);
    GlobalRadiant().signal_radiantStarted().connect(
        sigc::mem_fun(*_startupMapLoader, &StartupMapLoader::onRadiantStartup)
    );
    GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(*_startupMapLoader, &StartupMapLoader::onRadiantShutdown)
    );

    // Add the Map-related commands to the EventManager
    registerCommands();

    // Add the region-related commands to the EventManager
    RegionManager::initialiseCommands();

    // Add the map position commands to the EventManager
    GlobalMapPosition().initialise();

	MapFileManager::registerFileTypes();
}

// Creates the static module instance
module::StaticModule<Map> staticMapModule;

} // namespace map

// Accessor method containing the singleton Map instance
map::Map& GlobalMap() 
{
    return *map::staticMapModule.getModule();
}
