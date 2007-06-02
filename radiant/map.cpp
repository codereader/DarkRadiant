/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "map.h"

#include "debugging/debugging.h"

#include "ieventmanager.h"
#include "iselection.h"
#include "igroupnode.h"
#include "icounter.h"
#include "iundo.h"
#include "brush/TexDef.h"
#include "ibrush.h"
#include "ifilter.h"
#include "ireference.h"
#include "ifiletypes.h"
#include "ieclass.h"
#include "irender.h"
#include "ientity.h"
#include "editable.h"
#include "ifilesystem.h"
#include "inamespace.h"

#include <set>

#include <gtk/gtkmain.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktable.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcellrenderertext.h>
#include <gdk/gdkkeysyms.h>

#include "scenelib.h"
#include "transformlib.h"
#include "selectionlib.h"
#include "instancelib.h"
#include "traverselib.h"
#include "entitylib.h"
#include "cmdlib.h"
#include "stream/textfilestream.h"
#include "os/path.h"
#include "uniquenames.h"
#include "stream/stringstream.h"
#include "gtkutil/messagebox.h"
#include "convert.h"

#include "timer.h"
#include "select.h"
#include "plugin.h"
#include "filetypes.h"
#include "gtkdlgs.h"
#include "environment.h"
#include "mainframe.h"
#include "gtkutil/dialog.h"
#include "referencecache.h"
#include "brush/BrushNode.h"
#include "brush/BrushModule.h"
#include "camera/CamWnd.h"
#include "camera/GlobalCamera.h"
#include "xyview/GlobalXYWnd.h"
#include "map/MapPositionManager.h"
#include "ui/mru/MRU.h"
#include "map/AutoSaver.h"
#include "map/MapFileManager.h"
#include "map/RegionManager.h"
#include "map/RootNode.h"
#include "map/PointFile.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "selection/algorithm/Primitives.h"
#include "namespace/Namespace.h"

#include <string>
#include <boost/lexical_cast.hpp>

	namespace {
		const std::string MAP_UNNAMED_STRING = "unnamed.map";
		
		const std::string RKEY_LAST_CAM_POSITION = "game/mapFormat/lastCameraPositionKey";
		const std::string RKEY_LAST_CAM_ANGLE = "game/mapFormat/lastCameraAngleKey";
		const std::string RKEY_PLAYER_START_ECLASS = "game/mapFormat/playerStartPoint";
	}

Map::Map() : 
	m_resource(ReferenceCache::ResourcePtr()),
	m_valid(false)
{}

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

	gtk_window_set_title(MainFrame_getWindow(), title.c_str());
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
	std::string moduleName = findModuleName(GetFileTypeRegistry(), 
											std::string(MapFormat::Name()), 
											path_get_extension(filename.c_str()));
											
	MapFormat* format = Radiant_getMapModules().findModule(moduleName.c_str());
	ASSERT_MESSAGE(format != 0, "map format not found for file " << makeQuoted(filename.c_str()));
	return *format;
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

// Accessor method containing the singleton Map instance
Map& GlobalMap() {
	static Map _mapInstance;
	return _mapInstance;
}

namespace map {

class AABBCollectorVisible : 
	public scene::Graph::Walker
{
	AABB& _targetAABB;
public:
	AABBCollectorVisible(AABB& targetAABB) :
		_targetAABB(targetAABB)
	{}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if (path.top()->visible()) {
			_targetAABB.includeAABB(instance.worldAABB());
		}
		return true;
	}
};

AABB getVisibleBounds() {
	AABB returnValue;
	
	GlobalSceneGraph().traverse(AABBCollectorVisible(returnValue));
	
	return returnValue;
}

} // namespace map

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

/* Check if a node is the worldspawn.
 */
inline bool node_is_worldspawn(scene::INodePtr node) {
	Entity* entity = Node_getEntity(node);
	return entity != 0 && entity->getKeyValue("classname") == "worldspawn";
}

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

scene::INodePtr Map::findWorldspawn() {
	// Clear the current worldspawn node
	setWorldspawn(scene::INodePtr());

	// Traverse the scenegraph and search for the worldspawn
	Node_getTraversable(GlobalSceneGraph().root())->traverse(MapWorldspawnFinder());

	return getWorldspawn();
}


class CollectAllWalker : 
	public scene::Traversable::Walker
{
  scene::INodePtr m_root;
  UnsortedNodeSet& m_nodes;
public:
  CollectAllWalker(scene::INodePtr root, UnsortedNodeSet& nodes) : 
  	m_root(root), 
  	m_nodes(nodes)
  {}
  
  bool pre(scene::INodePtr node) const {
    m_nodes.insert(node);
    Node_getTraversable(m_root)->erase(node);
    return false;
  }
};

void Node_insertChildFirst(scene::INodePtr parent, scene::INodePtr child)
{
  UnsortedNodeSet nodes;
  Node_getTraversable(parent)->traverse(CollectAllWalker(parent, nodes));
  Node_getTraversable(parent)->insert(child);

  for(UnsortedNodeSet::iterator i = nodes.begin(); i != nodes.end(); ++i)
  {
    Node_getTraversable(parent)->insert((*i));
  }
}

scene::INodePtr createWorldspawn()
{
  scene::INodePtr worldspawn(GlobalEntityCreator().createEntity(GlobalEntityClassManager().findOrInsert("worldspawn", true)));
  Node_insertChildFirst(GlobalSceneGraph().root(), worldspawn);
  return worldspawn;
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


class MapMergeAll : public scene::Traversable::Walker
{
  mutable scene::Path m_path;
public:
  MapMergeAll(const scene::Path& root)
    : m_path(root)
  {
  }
  bool pre(scene::INodePtr node) const
  {
    Node_getTraversable(m_path.top())->insert(node);
    m_path.push(node);
    selectPath(m_path, true);
    return false;
  }
  void post(scene::INodePtr node) const
  {
    m_path.pop();
  }
};

class MapMergeEntities : 
	public scene::Traversable::Walker
{
	// The target path (usually GlobalSceneGraph().root())
	mutable scene::Path m_path;

public:
	MapMergeEntities(const scene::Path& root) : 
		m_path(root) 
	{}

	bool pre(scene::INodePtr node) const {
		// greebo: Check if the visited node is the worldspawn of the other map
		if (node_is_worldspawn(node)) {
			// Find the worldspawn of the target map
			scene::INodePtr world_node = GlobalMap().findWorldspawn();
			
			if (world_node == NULL) {
				// Set the worldspawn to the new node
				GlobalMap().setWorldspawn(node);
				
				// Insert the visited node at the target path
				Node_getTraversable(m_path.top())->insert(node);
				
				m_path.push(node);
				
				// Select all the children of the visited node (these are primitives)
				Node_getTraversable(node)->traverse(SelectChildren(m_path));
			}
			else {
				m_path.push(world_node);
				Node_getTraversable(node)->traverse(MapMergeAll(m_path));
			}
		}
		else {
			// This is an ordinary entity, not worldspawn
			
			// Insert this node at the target path 
			Node_getTraversable(m_path.top())->insert(node);
			m_path.push(node);
			
			// greebo: commented this out, we don't want the child brushes to be selected
			/*if (node_is_group(node)) {
				Node_getTraversable(node)->traverse(SelectChildren(m_path));
			}
			else {
				selectPath(m_path, true);
			}*/
			
			// Select the visited instance
			selectPath(m_path, true);
		}
		
		// Only traverse top-level entities, don't traverse the children
		return false;
	}

  void post(scene::INodePtr node) const
  {
    m_path.pop();
  }
};

class BasicContainer : 
	public scene::Node,
	public TraversableNodeSet // implements scene::Traversable
{
public:
	BasicContainer()
	{}
};

/// Merges the map graph rooted at \p node into the global scene-graph.
void MergeMap(scene::INodePtr node)
{
  Node_getTraversable(node)->traverse(MapMergeEntities(scene::Path(GlobalSceneGraph().root())));
}
void Map_ImportSelected(TextInputStream& in, const MapFormat& format)
{
	scene::INodePtr node(new BasicContainer);
	format.readGraph(node, in, GlobalEntityCreator());
	GlobalNamespace().gatherNamespaced(node);
	GlobalNamespace().mergeClonedNames();
	MergeMap(node);
}

inline scene::CloneablePtr Node_getCloneable(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<scene::Cloneable>(node);
}

inline scene::INodePtr node_clone(scene::INodePtr node)
{
	scene::CloneablePtr cloneable = Node_getCloneable(node);
	if (cloneable != NULL) {
		return cloneable->clone();
	}
  
	// Return an empty node
	return scene::INodePtr(new scene::Node);
}

class CloneAll : 
	public scene::Traversable::Walker
{
	mutable scene::Path m_path;
public:
	CloneAll(scene::INodePtr root) : 
		m_path(root)
	{}
	
  bool pre(scene::INodePtr node) const
  {
    if (node->isRoot()) {
      return false;
    }
    
    m_path.push(node_clone(node));
    //m_path.top().get().IncRef();

    return true;
  }
  void post(scene::INodePtr node) const
  {
    if(node->isRoot()) {
      return;
    }

    Node_getTraversable(m_path.parent())->insert(m_path.top());

    //m_path.top().get().DecRef();
    m_path.pop();
  }
};

scene::INodePtr Node_Clone(scene::INodePtr node)
{
  scene::INodePtr clone = node_clone(node);
  scene::TraversablePtr traversable = Node_getTraversable(node);
  if(traversable != NULL)
  {
    traversable->traverse(CloneAll(clone));
  }
  return clone;
}

class ScopeTimer
{
  Timer m_timer;
  const char* m_message;
public:
  ScopeTimer(const char* message)
    : m_message(message)
  {
    m_timer.start();
  }
  ~ScopeTimer()
  {
    double elapsed_time = m_timer.elapsed_msec() / 1000.f;
    globalOutputStream() << m_message << " timer: " << FloatFormat(elapsed_time, 5, 2) << " second(s) elapsed\n";
  }
};

void Map::load(const std::string& filename) {
	globalOutputStream() << "Loading map from " << filename << "\n";

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
	
	// Clear the modified flag
	setModified(false);
}

class Excluder
{
public:
  virtual bool excluded(scene::INodePtr node) const = 0;
};

class ExcludeWalker : public scene::Traversable::Walker
{
  const scene::Traversable::Walker& m_walker;
  const Excluder* m_exclude;
  mutable bool m_skip;
public:
  ExcludeWalker(const scene::Traversable::Walker& walker, const Excluder& exclude)
    : m_walker(walker), m_exclude(&exclude), m_skip(false)
  {
  }
  bool pre(scene::INodePtr node) const
  {
    if(m_exclude->excluded(node) || node->isRoot())
    {
      m_skip = true;
      return false;
    }
    else
    {
      m_walker.pre(node);
    }
    return true;
  }
  void post(scene::INodePtr node) const
  {
    if(m_skip)
    {
      m_skip = false;
    }
    else
    {
      m_walker.post(node);
    }
  }
};

class AnyInstanceSelected : public scene::Instantiable::Visitor
{
  bool& m_selected;
public:
  AnyInstanceSelected(bool& selected) : m_selected(selected)
  {
    m_selected = false;
  }
  void visit(scene::Instance& instance) const
  {
    Selectable* selectable = Instance_getSelectable(instance);
    if(selectable != 0
      && selectable->isSelected())
    {
      m_selected = true;
    }
  }
};

bool Node_instanceSelected(scene::INodePtr node)
{
  scene::InstantiablePtr instantiable = Node_getInstantiable(node);
  ASSERT_NOTNULL(instantiable);
  bool selected;
  instantiable->forEachInstance(AnyInstanceSelected(selected));
  return selected;
}

class SelectedDescendantWalker : public scene::Traversable::Walker
{
  bool& m_selected;
public:
  SelectedDescendantWalker(bool& selected) : m_selected(selected)
  {
    m_selected = false;
  }

  bool pre(scene::INodePtr node) const
  {
    if(node->isRoot())
    {
      return false;
    }

    if(Node_instanceSelected(node))
    {
      m_selected = true;
    }

    return true;
  }
};

bool Node_selectedDescendant(scene::INodePtr node)
{
  bool selected;
  Node_traverseSubgraph(node, SelectedDescendantWalker(selected));
  return selected;
}

class SelectionExcluder : public Excluder
{
public:
  bool excluded(scene::INodePtr node) const
  {
    return !Node_selectedDescendant(node);
  }
};

class IncludeSelectedWalker : public scene::Traversable::Walker
{
  const scene::Traversable::Walker& m_walker;
  mutable std::size_t m_selected;
  mutable bool m_skip;

  bool selectedParent() const
  {
    return m_selected != 0;
  }
public:
  IncludeSelectedWalker(const scene::Traversable::Walker& walker)
    : m_walker(walker), m_selected(0), m_skip(false)
  {
  }
  bool pre(scene::INodePtr node) const
  {
    // include node if:
    // node is not a 'root' AND ( node is selected OR any child of node is selected OR any parent of node is selected )
    if(!node->isRoot() && (Node_selectedDescendant(node) || selectedParent()))
    {
      if(Node_instanceSelected(node))
      {
        ++m_selected;
      }
      m_walker.pre(node);
      return true;
    }
    else
    {
      m_skip = true;
      return false;
    }
  }
  void post(scene::INodePtr node) const
  {
    if(m_skip)
    {
      m_skip = false;
    }
    else
    {
      if(Node_instanceSelected(node))
      {
        --m_selected;
      }
      m_walker.post(node);
    }
  }
};

void Map_Traverse_Selected(scene::INodePtr root, const scene::Traversable::Walker& walker)
{
  scene::TraversablePtr traversable = Node_getTraversable(root);
  if(traversable != NULL) {
#if 0
    traversable->traverse(ExcludeWalker(walker, SelectionExcluder()));
#else
    traversable->traverse(IncludeSelectedWalker(walker));
#endif
  }
}

void Map_ExportSelected(std::ostream& out, const MapFormat& format)
{
	format.writeGraph(GlobalSceneGraph().root(), Map_Traverse_Selected, out);
}

void Map_Traverse(scene::INodePtr root, const scene::Traversable::Walker& walker)
{
  scene::TraversablePtr traversable = Node_getTraversable(root);
  if(traversable != NULL) {
    traversable->traverse(walker);
  }
}

void Map_RenameAbsolute(const char* absolute)
{
	ReferenceCache::ResourcePtr resource = 
		GlobalReferenceCache().capture(absolute);
		
  scene::INodePtr clone(NewMapRoot(path_make_relative(absolute, GlobalFileSystem().findRoot(absolute))));
  resource->setNode(clone);

  {
    //ScopeTimer timer("clone subgraph");
    Node_getTraversable(GlobalSceneGraph().root())->traverse(CloneAll(clone));
  }

  GlobalMap().m_resource->detach(GlobalMap());
  GlobalReferenceCache().release(GlobalMap().m_name.c_str());

	GlobalMap().m_resource = resource;

	GlobalMap().setName(absolute);

  GlobalMap().m_resource->attach(GlobalMap());
}

void Map_Rename(const std::string& filename)
{
	if(GlobalMap().m_name != filename) {
    	ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Saving Map");

    	Map_RenameAbsolute(filename.c_str());
    
    	SceneChangeNotify();
	}
	else {
    	SaveReferences();
  	}
}

void Map::save() {
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
}

void Map::createNew() {
	setName(MAP_UNNAMED_STRING);

	m_resource = GlobalReferenceCache().capture(m_name.c_str());
	m_resource->attach(*this);
	
	SceneChangeNotify();
	
	focusViews(Vector3(0,0,0), Vector3(0,0,0));
}

inline void exclude_node(scene::INodePtr node, bool exclude)
{
  exclude
    ? node->enable(scene::Node::eExcluded)
    : node->disable(scene::Node::eExcluded);
}

class ExcludeAllWalker : public scene::Graph::Walker
{
  bool m_exclude;
public:
  ExcludeAllWalker(bool exclude)
    : m_exclude(exclude)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    exclude_node(path.top(), m_exclude);

    return true;
  }
};

void Scene_Exclude_All(bool exclude)
{
  GlobalSceneGraph().traverse(ExcludeAllWalker(exclude));
}

bool Instance_isSelected(const scene::Instance& instance)
{
  const Selectable* selectable = Instance_getSelectable(instance);
  return selectable != 0 && selectable->isSelected();
}

class ExcludeSelectedWalker : public scene::Graph::Walker
{
  bool m_exclude;
public:
  ExcludeSelectedWalker(bool exclude)
    : m_exclude(exclude)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    exclude_node(path.top(), (instance.isSelected() || instance.childSelected() || instance.parentSelected()) == m_exclude);
    return true;
  }
};

void Scene_Exclude_Selected(bool exclude)
{
  GlobalSceneGraph().traverse(ExcludeSelectedWalker(exclude));
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
	MapResource_saveFile(getFormatForFile(filename), GlobalSceneGraph().root(), Map_Traverse, filename.c_str()); 
}

bool Map::saveSelected(const std::string& filename) {
	// Substract the origin from child primitives (of entities like func_static)
	selection::algorithm::removeOriginFromChildPrimitives();
	
	bool success = MapResource_saveFile(Map::getFormatForFile(filename), 
  							  GlobalSceneGraph().root(), 
  							  Map_Traverse_Selected, 
  							  filename.c_str());

	// Add the origin to all the children of func_static, etc.
	selection::algorithm::addOriginToChildPrimitives();

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
	std::string filename = map::MapFileManager::getMapFilename(false, "Save Map");
  
	if (!filename.empty()) {
	    GlobalMRU().insert(filename);
	    Map_Rename(filename);
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
	else if(GlobalMap().isModified()) {
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

class MapEntityClasses : public ModuleObserver
{
  std::size_t m_unrealised;
public:
  MapEntityClasses() : m_unrealised(1)
  {
  }
  void realise()
  {
    if(--m_unrealised == 0)
    {
      if(GlobalMap().m_resource != 0)
      {
	      GlobalMap().m_resource->realise();
      }
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
      if(GlobalMap().m_resource != 0)
      {
        GlobalMap().m_resource->flush();
	      GlobalMap().m_resource->unrealise();
      }
    }
  }
};

MapEntityClasses g_MapEntityClasses;

#include "preferencesystem.h"

void Map_Construct() {
	// Add the Map-related commands to the EventManager
	Map::registerCommands();
	
	// Add the region-related commands to the EventManager
	map::RegionManager::initialiseCommands();

	// Add the map position commands to the EventManager
	map::GlobalMapPosition().initialise();

	GlobalEntityClassManager().attach(g_MapEntityClasses);
}

void Map_Destroy()
{
  GlobalEntityClassManager().detach(g_MapEntityClasses);
}
