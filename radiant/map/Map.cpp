#include "Map.h"

#include "itextstream.h"
#include "iscenegraph.h"
#include "ieventmanager.h"
#include "iundo.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "ifilter.h"
#include "icounter.h"
#include "iradiant.h"
#include "inamespace.h"

#include "stream/stringstream.h"
#include "stream/textfilestream.h"
#include "traverselib.h"
#include "entitylib.h"
#include "convert.h"
#include "os/path.h"
#include "gtkutil/messagebox.h"

#include "mainframe.h"
#include "referencecache.h"
#include "plugin.h"
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
#include "map/algorithm/Clone.h"
#include "map/algorithm/Merge.h"
#include "map/algorithm/Traverse.h"
#include "ui/mru/MRU.h"
#include "selection/algorithm/Primitives.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "modulesystem/ModuleRegistry.h"

namespace map {
	
	namespace {
		const std::string MAP_UNNAMED_STRING = "unnamed.map";
		
		const std::string RKEY_LAST_CAM_POSITION = "game/mapFormat/lastCameraPositionKey";
		const std::string RKEY_LAST_CAM_ANGLE = "game/mapFormat/lastCameraAngleKey";
		const std::string RKEY_PLAYER_START_ECLASS = "game/mapFormat/playerStartPoint";
		const std::string RKEY_PLAYER_HEIGHT = "game/defaults/playerHeight";
		
		// Traverse all entities and store the first worldspawn into the map
		class MapWorldspawnFinder : 
			public scene::Traversable::Walker
		{
		public:
			bool pre(scene::INodePtr node) const {
				if (node_is_worldspawn(node)) {
					if (GlobalMap().getWorldspawn() == NULL) {
						GlobalMap().setWorldspawn(node);
					}
				}
				return false;
			}
		};
		
		class CollectAllWalker : 
			public scene::Traversable::Walker
		{
		  scene::INodePtr m_root;
		  std::vector<scene::INodePtr>& m_nodes;
		public:
		  CollectAllWalker(scene::INodePtr root, std::vector<scene::INodePtr>& nodes) : 
		  	m_root(root), 
		  	m_nodes(nodes)
		  {}
		  
		  bool pre(scene::INodePtr node) const {
		  	m_nodes.push_back(node);
		    Node_getTraversable(m_root)->erase(node);
		    return false;
		  }
		};
		
		void Node_insertChildFirst(scene::INodePtr parent, scene::INodePtr child) {
			// Create a container to collect all the existing entities in the scenegraph
			std::vector<scene::INodePtr> nodes;
			
			// The parent has to be a Traversable node
			scene::TraversablePtr traversable = Node_getTraversable(parent);
			assert(traversable);
			
			// Collect all the child nodes of <parent> and move them into the container
			traversable->traverse(CollectAllWalker(parent, nodes));
			// Now that the <parent> is empty, insert the worldspawn as first child
			traversable->insert(child);
		
			// Insert all the nodes again
			for (std::vector<scene::INodePtr>::iterator i = nodes.begin(); 
				 i != nodes.end(); 
				 ++i)
			{
				Node_getTraversable(parent)->insert(*i);
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
	m_resource(ReferenceCache::ResourcePtr()),
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
		m_resource->flush();
		m_resource->unrealise();
	}
}

void Map::realise() {
    if(m_resource != 0)
    {
      if (isUnnamed()) {
        m_resource->setNode(NewMapRoot(""));
        MapFilePtr map = Node_getMapFile(m_resource->getNode());
        if(map != NULL) {
          map->save();
        }
      }
      else
      {
        m_resource->load();
      }

      GlobalSceneGraph().insert_root(m_resource->getNode());

      map::AutoSaver().clearChanges();

      setValid(true);
    }
}

void Map::unrealise() {
    if(m_resource != 0)
    {
      setValid(false);
      setWorldspawn(scene::INodePtr());

      GlobalUndoSystem().clear();

      GlobalSceneGraph().erase_root();
    }
}
  
void Map::setValid(bool valid) {
	m_valid = valid;
	m_mapValidCallbacks();
}

bool Map::isValid() const {
	return m_valid;
}

void Map::addValidCallback(const SignalHandler& handler) {
	m_mapValidCallbacks.connectLast(handler);
}

void Map::updateTitle() {
	StringOutputStream title;
	title << ConvertLocaleToUTF8(m_name.c_str());

	if (m_modified) {
		title << " *";
	}

	gtk_window_set_title(GlobalRadiant().getMainWindow(), title.c_str());
}

void Map::setName(const std::string& newName) {
	m_name = newName;
	updateTitle();
}

std::string Map::getName() const {
	return m_name;
}

bool Map::isUnnamed() const {
	return m_name == MAP_UNNAMED_STRING;
}

void Map::setWorldspawn(scene::INodePtr node) {
	m_world_node = node;
}

scene::INodePtr Map::getWorldspawn() {
	return m_world_node;
}

const MapFormat& Map::getFormatForFile(const std::string& filename) {
	// Look up the module name which loads the given extension
	std::string moduleName = GlobalFiletypes().findModuleName("map", 
											path_get_extension(filename.c_str()));
											
	// Acquire the module from the ModuleRegistry
	RegisterableModulePtr mapLoader = module::GlobalModuleRegistry().getModule(moduleName);
	
	ASSERT_MESSAGE(mapLoader != NULL, "map format not found for file " << makeQuoted(filename.c_str()));
	return *static_cast<MapFormat*>(mapLoader.get());
}

const MapFormat& Map::getFormat() {
	return getFormatForFile(m_name);
}

// free all map elements, reinitialize the structures that depend on them
void Map::free() {
	map::PointFile::Instance().clear();

	GlobalShaderClipboard().clear();

	m_resource->detach(*this);
	GlobalReferenceCache().release(m_name.c_str());

	// Reset the resource pointer
	m_resource = ReferenceCache::ResourcePtr();

	FlushReferences();
}

bool Map::isModified() const {
	return m_modified;
}

// Set the modified flag
void Map::setModified(bool modifiedFlag) {
	m_modified = modifiedFlag;
    updateTitle();
}

// move the view to a certain position
void Map::focusViews(const Vector3& point, const Vector3& angles) {
	// Set the camera and the views to the given point
	GlobalCamera().focusCamera(point, angles);
	GlobalXYWnd().setOrigin(point);
}

void Map::removeCameraPosition() {
	const std::string keyLastCamPos = GlobalRegistry().get(RKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = GlobalRegistry().get(RKEY_LAST_CAM_ANGLE);
	
	if (m_world_node != NULL) {
		// Retrieve the entity from the worldspawn node
		Entity* worldspawn = Node_getEntity(m_world_node);
		assert(worldspawn != NULL);	// This must succeed
		
		worldspawn->setKeyValue(keyLastCamPos, "");
		worldspawn->setKeyValue(keyLastCamAngle, "");
	}
}

/* greebo: Saves the current camera position/angles to worldspawn
 */
void Map::saveCameraPosition() {
	const std::string keyLastCamPos = GlobalRegistry().get(RKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = GlobalRegistry().get(RKEY_LAST_CAM_ANGLE); 
	
	if (m_world_node != NULL) {
		// Retrieve the entity from the worldspawn node
		Entity* worldspawn = Node_getEntity(m_world_node);
		assert(worldspawn != NULL);	// This must succeed
		
		CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
		
		worldspawn->setKeyValue(keyLastCamPos, camwnd.getCameraOrigin());
		worldspawn->setKeyValue(keyLastCamAngle, camwnd.getCameraAngles());
	}
}

/** Find the start position in the map and focus the viewport on it.
 */
void Map::gotoStartPosition() {
	const std::string keyLastCamPos = GlobalRegistry().get(RKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = GlobalRegistry().get(RKEY_LAST_CAM_ANGLE); 
	const std::string eClassPlayerStart = GlobalRegistry().get(RKEY_PLAYER_START_ECLASS);
	
	Vector3 angles(0,0,0);
	Vector3 origin(0,0,0);
	
	if (m_world_node != NULL) {
		// Retrieve the entity from the worldspawn node
		Entity* worldspawn = Node_getEntity(m_world_node);
		assert(worldspawn != NULL);	// This must succeed
		
		// Try to find a saved "last camera position"
		const std::string savedOrigin = worldspawn->getKeyValue(keyLastCamPos);
		
		if (savedOrigin != "") {
			// Construct the vector out of the std::string
			origin = Vector3(savedOrigin);
			
			Vector3 angles = worldspawn->getKeyValue(keyLastCamAngle);
			
			// Focus the view with the default angle
			focusViews(origin, angles);
			
			// Remove the saved entity key value so it doesn't appear during map edit
			removeCameraPosition();
			
			return;
		}
		else {
			// Get the player start entity
			Entity* playerStart = Scene_FindEntityByClass(eClassPlayerStart);
			
			if (playerStart != NULL) {
				// Get the entity origin
				origin = playerStart->getKeyValue("origin");
				
				// angua: move the camera upwards a bit
				origin.z() += GlobalRegistry().getFloat(RKEY_PLAYER_HEIGHT);
				
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
	Node_getTraversable(GlobalSceneGraph().root())->traverse(MapWorldspawnFinder());

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
	globalOutputStream() << "Loading map from " << filename.c_str() << "\n";

	setName(filename);

	{
		ScopeTimer timer("map load");

		m_resource = GlobalReferenceCache().capture(m_name);
		m_resource->attach(*this);

		// Get the traversable root
		scene::TraversablePtr rt = Node_getTraversable(GlobalSceneGraph().root());
		assert(rt != NULL);
	
		// Traverse the scenegraph and find the worldspawn 
		rt->traverse(MapWorldspawnFinder());
	}

	globalOutputStream() << "--- LoadMapFile ---\n";
	globalOutputStream() << m_name.c_str() << "\n";
  
	globalOutputStream() << makeLeftJustified(Unsigned(GlobalRadiant().getCounter(counterBrushes).get()), 5) << " brushes\n";
	globalOutputStream() << makeLeftJustified(Unsigned(GlobalRadiant().getCounter(counterPatches).get()), 5) << " patches\n";
	globalOutputStream() << makeLeftJustified(Unsigned(GlobalRadiant().getCounter(counterEntities).get()), 5) << " entities\n";

	// Add the origin to all the children of func_static, etc.
	selection::algorithm::addOriginToChildPrimitives();

	// Move the view to a start position
	gotoStartPosition();

	// Load the stored map positions from the worldspawn entity
	map::GlobalMapPosition().loadPositions();
	// Remove them, so that the user doesn't get bothered with them
	map::GlobalMapPosition().removePositions();
	
	// Disable the region to make sure
	GlobalRegion().disable();
	
	// Clear the shaderclipboard, the references are most probably invalid now
	GlobalShaderClipboard().clear();

	// Let the filtersystem update the filtered status of all instances
	GlobalFilterSystem().update();
	
	// Clear the modified flag
	setModified(false);
}

void Map::save() {
	if (_saveInProgress) return; // safeguard
	
	_saveInProgress = true;
	
	ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Saving Map");
	
	// Store the camview position into worldspawn
	saveCameraPosition();
	
	// Store the map positions into the worldspawn spawnargs
	map::GlobalMapPosition().savePositions();
	
	map::PointFile::Instance().clear();

	ScopeTimer timer("map save");
	
	// Substract the origin from child primitives (of entities like func_static)
	selection::algorithm::removeOriginFromChildPrimitives();
	
	// Save the actual map, by iterating through the reference cache and saving
	// each ModelResource.
	SaveReferences();
	
	// Re-add the origins to the child primitives (of entities like func_static)
	selection::algorithm::addOriginToChildPrimitives();
  
	// Remove the saved camera position
	removeCameraPosition();
	
	// Remove the map positions again after saving
	map::GlobalMapPosition().removePositions();
	
	// Clear the modified flag
	setModified(false);

	_saveInProgress = false;
}

void Map::createNew() {
	setName(MAP_UNNAMED_STRING);

	m_resource = GlobalReferenceCache().capture(m_name.c_str());
	m_resource->attach(*this);
	
	SceneChangeNotify();
	
	focusViews(Vector3(0,0,0), Vector3(0,0,0));
}

bool Map::import(const std::string& filename) {
	bool success = false;
	
	{
		ReferenceCache::ResourcePtr resource = 
			GlobalReferenceCache().capture(filename);
		
		// avoid loading old version if map has changed on disk since last import
		resource->refresh(); 
		
		if (resource->load()) {
			scene::INodePtr clone(NewMapRoot(""));

			{
				bool textureLockStatus = GlobalBrush()->textureLockEnabled();
				GlobalBrush()->setTextureLock(false);
				
				// Add the origin to all the child brushes
				Node_getTraversable(resource->getNode())->traverse(
					selection::algorithm::OriginAdder()
				);
        
				Node_getTraversable(resource->getNode())->traverse(
					CloneAll(clone)
				);
				
				GlobalBrush()->setTextureLock(textureLockStatus);
			}

			GlobalNamespace().gatherNamespaced(clone);
			GlobalNamespace().mergeClonedNames();
			MergeMap(clone);
			success = true;
		}
		
		GlobalReferenceCache().release(filename);
	}

	SceneChangeNotify();

	return success;
}

void Map::saveDirect(const std::string& filename) {
	if (_saveInProgress) return; // safeguard

	ScopeDisableScreenUpdates disableScreenUpdates(path_get_filename_start(filename.c_str()));
	
	_saveInProgress = true;

	// Substract the origin from child primitives (of entities like func_static)
	selection::algorithm::removeOriginFromChildPrimitives();
	
	MapResource_saveFile(
		getFormatForFile(filename), 
		GlobalSceneGraph().root(), 
		map::traverse, // TraversalFunc 
		filename.c_str()
	);
	
	// Re-add the origins to the child primitives (of entities like func_static)
	selection::algorithm::addOriginToChildPrimitives();

	_saveInProgress = false;
}

bool Map::saveSelected(const std::string& filename) {
	if (_saveInProgress) return false; // safeguard

	ScopeDisableScreenUpdates disableScreenUpdates(path_get_filename_start(filename.c_str()));
	
	_saveInProgress = true;

	// Substract the origin from child primitives (of entities like func_static)
	selection::algorithm::removeOriginFromChildPrimitives();
	
	bool success = MapResource_saveFile(Map::getFormatForFile(filename), 
  							  GlobalSceneGraph().root(), 
  							  map::traverseSelected, // TraversalFunc
  							  filename.c_str());

	// Add the origin to all the children of func_static, etc.
	selection::algorithm::addOriginToChildPrimitives();

	_saveInProgress = false;

	return success;
}

bool Map::askForSave(const std::string& title) {
	if (!isModified()) {
		// Map is not modified, return positive
		return true;
	}

	// Ask the user
	EMessageBoxReturn result = gtk_MessageBox(
		GTK_WIDGET(GlobalRadiant().getMainWindow()), 
		"The current map has changed since it was last saved."
		"\nDo you want to save the current map before continuing?", 
		title.c_str(), 
		eMB_YESNOCANCEL, eMB_ICONQUESTION
	);
	
	if (result == eIDCANCEL) {
		return false;
	}
	
	if (result == eIDYES) {
		// The user wants to save the map
	    if (isUnnamed()) {
	    	// Map still unnamed, try to save the map with a new name
	    	// and take the return value from the other routine.
			return saveAs();
	    }
	    else {
	    	// Map is named, save it
			save();
	    }
	}

	// Default behaviour: allow the save
	return true;
}

bool Map::saveAs() {
	if (_saveInProgress) return false; // safeguard

	std::string filename = map::MapFileManager::getMapFilename(false, "Save Map", "map", getName());
  
	if (!filename.empty()) {
	    GlobalMRU().insert(filename);
	    rename(filename);
	    save();
	    return true;
	}
	else {
		// Invalid filename entered, return false
		return false;
	}
}

void Map::loadPrefabAt(const Vector3& targetCoords) {
	std::string filename = map::MapFileManager::getMapFilename(true, "Load Prefab", "prefab");
	
	if (!filename.empty()) {
		UndoableCommand undo("loadPrefabAt");
	    
	    // Deselect everything
	    GlobalSelectionSystem().setSelectedAll(false);
	    
	    // Now import the prefab (imported items get selected)
	    import(filename);
	    
	    // Translate the selection to the given point
	    GlobalSelectionSystem().translateSelected(targetCoords);
	}
}

void Map::registerCommands() {
	GlobalEventManager().addCommand("NewMap", FreeCaller<Map::newMap>());
	GlobalEventManager().addCommand("OpenMap", FreeCaller<Map::openMap>());
	GlobalEventManager().addCommand("ImportMap", FreeCaller<Map::importMap>());
	GlobalEventManager().addCommand("LoadPrefab", FreeCaller<Map::loadPrefab>());
	GlobalEventManager().addCommand("SaveSelectedAsPrefab", FreeCaller<Map::saveSelectedAsPrefab>());
	GlobalEventManager().addCommand("SaveMap", FreeCaller<Map::saveMap>());
	GlobalEventManager().addCommand("SaveMapAs", FreeCaller<Map::saveMapAs>());
	GlobalEventManager().addCommand("SaveSelected", FreeCaller<Map::exportMap>());
}

// Static command targets
void Map::newMap() {
	if (GlobalMap().askForSave("New Map")) {
		// Turn regioning off when starting a new map
		GlobalRegion().disable();

		GlobalMap().free();
		GlobalMap().createNew();
	}
}

void Map::openMap() {
	if (!GlobalMap().askForSave("Open Map"))
		return;

	// Get the map file name to load
	std::string filename = map::MapFileManager::getMapFilename(true, 
															   "Open map");

	if (!filename.empty()) {
	    GlobalMRU().insert(filename);
	    
	    GlobalMap().free();
	    GlobalMap().load(filename);
	}
}

void Map::importMap() {
	std::string filename = map::MapFileManager::getMapFilename(true,
															   "Import map");

	if (!filename.empty()) {
	    UndoableCommand undo("mapImport");
	    GlobalMap().import(filename);
	}
}

void Map::saveMapAs() {
	GlobalMap().saveAs();
}

void Map::saveMap() {
	if (GlobalMap().isUnnamed()) {
		GlobalMap().saveAs();
	}
	// greebo: Always let the map be saved, regardless of the modified status.
	else /*if(GlobalMap().isModified())*/ {
		GlobalMap().save();
	}
}

void Map::exportMap() {
	std::string filename = map::MapFileManager::getMapFilename(
								false, "Export selection");

	if (!filename.empty()) {
	    GlobalMap().saveSelected(filename);
  	}
}

void Map::loadPrefab() {
	GlobalMap().loadPrefabAt(Vector3(0,0,0));
}

void Map::saveSelectedAsPrefab() {
	std::string filename = 
		map::MapFileManager::getMapFilename(false, "Save selected as Prefab", "prefab");
	
	if (!filename.empty()) {
	    GlobalMap().saveSelected(filename);
  	}
}

void Map::renameAbsolute(const std::string& absolute) {
	ReferenceCache::ResourcePtr resource = 
		GlobalReferenceCache().capture(absolute);
		
	scene::INodePtr clone(
		NewMapRoot(
			path_make_relative(absolute.c_str(), GlobalFileSystem().findRoot(absolute.c_str()))
		)
	);
	resource->setNode(clone);

	Node_getTraversable(GlobalSceneGraph().root())->traverse(CloneAll(clone));

	m_resource->detach(*this);
	GlobalReferenceCache().release(m_name);

	m_resource = resource;

	setName(absolute);

	m_resource->attach(*this);

	// greebo: Somehow the filter settings get lost after a rename, trigger an update.
	GlobalFilterSystem().update();
}

void Map::rename(const std::string& filename) {
	if (m_name != filename) {
    	ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Saving Map");

    	renameAbsolute(filename);
    
    	SceneChangeNotify();
	}
	else {
    	SaveReferences();
  	}
}

void Map::importSelected(TextInputStream& in) {
	scene::INodePtr node(new BasicContainer);
	
	const MapFormat& format = getFormat();
	format.readGraph(node, in);
	
	GlobalNamespace().gatherNamespaced(node);
	GlobalNamespace().mergeClonedNames();
	
	MergeMap(node);
}

void Map::exportSelected(std::ostream& out) {
	const MapFormat& format = getFormat();
	format.writeGraph(GlobalSceneGraph().root(), map::traverseSelected, out);
}

} // namespace map

// Accessor method containing the singleton Map instance
map::Map& GlobalMap() {
	static map::Map _mapInstance;
	return _mapInstance;
}
