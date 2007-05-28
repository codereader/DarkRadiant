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
#include "imap.h"
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
#include "moduleobserver.h"

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
#include "signal/signal.h"
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
#include "namespace/Namespace.h"

#include <string>
#include <boost/lexical_cast.hpp>

	namespace {
		const std::string RKEY_LAST_CAM_POSITION = "game/mapFormat/lastCameraPositionKey";
		const std::string RKEY_LAST_CAM_ANGLE = "game/mapFormat/lastCameraAngleKey";
		const std::string RKEY_PLAYER_START_ECLASS = "game/mapFormat/playerStartPoint";
	}

class Map;
void Map_SetValid(Map& map, bool valid);
void Map_UpdateTitle(const Map& map);
void Map_SetWorldspawn(Map& map, scene::INodePtr node);


class Map : public ModuleObserver
{
public:
  std::string m_name;
	
	// Pointer to the Model Resource for this map
	ReferenceCache::ResourcePtr m_resource;
	
  bool m_valid;

  bool m_modified;
  void (*m_modified_changed)(const Map&);

  Signal0 m_mapValidCallbacks;

	scene::INodePtr m_world_node; // "classname" "worldspawn" !

	Map() 
	: m_valid(false), 
	  m_modified_changed(Map_UpdateTitle)
	{ }

  void realise()
  {
    if(m_resource != 0)
    {
      if(Map_Unnamed(*this))
      {
        g_map.m_resource->setNode(NewMapRoot(""));
        MapFilePtr map = Node_getMapFile(g_map.m_resource->getNode());
        if(map != 0)
        {
          map->save();
        }
      }
      else
      {
        m_resource->load();
      }

      GlobalSceneGraph().insert_root(m_resource->getNode());

      map::AutoSaver().clearChanges();

      Map_SetValid(g_map, true);
    }
  }
  void unrealise()
  {
    if(m_resource != 0)
    {
      Map_SetValid(g_map, false);
      Map_SetWorldspawn(g_map, scene::INodePtr());


      GlobalUndoSystem().clear();

      GlobalSceneGraph().erase_root();
    }
  }
};

Map g_map;

void Map_addValidCallback(Map& map, const SignalHandler& handler)
{
  map.m_mapValidCallbacks.connectLast(handler);
}

bool Map_Valid(const Map& map)
{
  return map.m_valid;
}

void Map_SetValid(Map& map, bool valid)
{
  map.m_valid = valid;
  map.m_mapValidCallbacks();
}

namespace map {

/* Return the name of the current map.
 */
std::string getFileName() {
	return g_map.m_name;
}	

// move the view to a start position
//
void focusViews(const Vector3& point, const Vector3& angles) {
	// Set the camera and the views to the given point
	GlobalCamera().focusCamera(point, angles);
	GlobalXYWnd().setOrigin(point);
}

class OriginRemover :
	public scene::Graph::Walker 
{
public:
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		Entity* entity = Node_getEntity(path.top());
		
		// Check for an entity
		if (entity != NULL) {
			// greebo: Check for a Doom3Group
			scene::GroupNodePtr groupNode = Node_getGroupNode(path.top());
			
			// Don't handle the worldspawn children, they're safe&sound
			if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn") {
				groupNode->removeOriginFromChildren();
				// Don't traverse the children
				return false;
			}
		}
		
		return true;
	}
};

class OriginAdder :
	public scene::Graph::Walker,
	public scene::Traversable::Walker
{
public:
	// Graph::Walker implementation
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		Entity* entity = Node_getEntity(path.top());
		
		// Check for an entity
		if (entity != NULL) {
			// greebo: Check for a Doom3Group
			scene::GroupNodePtr groupNode = Node_getGroupNode(path.top());
			
			// Don't handle the worldspawn children, they're safe&sound
			if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn") {
				groupNode->addOriginToChildren();
				// Don't traverse the children
				return false;
			}
		}
		
		return true;
	}
	
	// Traversable::Walker implementation
	bool pre(scene::INodePtr node) const {
		Entity* entity = Node_getEntity(node);
		
		// Check for an entity
		if (entity != NULL) {
			// greebo: Check for a Doom3Group
			scene::GroupNodePtr groupNode = Node_getGroupNode(node);
			
			// Don't handle the worldspawn children, they're safe&sound
			if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn") {
				groupNode->addOriginToChildren();
				// Don't traverse the children
				return false;
			}
		}
		return true;
	}

};

void removeOriginFromChildPrimitives() {
	bool textureLockStatus = GlobalBrush()->textureLockEnabled();
	GlobalBrush()->setTextureLock(false);
	GlobalSceneGraph().traverse(OriginRemover());
	GlobalBrush()->setTextureLock(textureLockStatus);
}

void addOriginToChildPrimitives() {
	bool textureLockStatus = GlobalBrush()->textureLockEnabled();
	GlobalBrush()->setTextureLock(false);
	GlobalSceneGraph().traverse(OriginAdder());
	GlobalBrush()->setTextureLock(textureLockStatus);
}

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

bool Map_Unnamed(const Map& map)
{
	return map::getFileName() == "unnamed.map";
}

const MapFormat& MapFormat_forFile(const std::string& filename) {
	// Look up the module name which loads the given extension
	std::string moduleName = findModuleName(GetFileTypeRegistry(), 
											std::string(MapFormat::Name()), 
											path_get_extension(filename.c_str()));
											
	MapFormat* format = Radiant_getMapModules().findModule(moduleName.c_str());
	ASSERT_MESSAGE(format != 0, "map format not found for file " << makeQuoted(filename.c_str()));
	return *format;
}

const MapFormat& Map_getFormat(const Map& map)
{
  return MapFormat_forFile(map::getFileName());
}


bool Map_Modified(const Map& map)
{
  return map.m_modified;
}

namespace map {

	// Set the modified flag
	void setModified(bool modifiedFlag) {
		g_map.m_modified = modifiedFlag;
	    g_map.m_modified_changed(g_map);
	}
	
}

// Moved from qe3.cpp to here
void Sys_SetTitle(const char *text, bool modified)
{
  StringOutputStream title;
  title << ConvertLocaleToUTF8(text);

  if(modified)
  {
    title << " *";
  }

  gtk_window_set_title(MainFrame_getWindow(), title.c_str());
}

void Map_UpdateTitle(const Map& map)
{
  Sys_SetTitle(map.m_name.c_str(), Map_Modified(map));
}



scene::INodePtr Map_GetWorldspawn(const Map& map) {
	return map.m_world_node;
}

void Map_SetWorldspawn(Map& map, scene::INodePtr node) {
	map.m_world_node = node;
}

/*
================
Map_Free
free all map elements, reinitialize the structures that depend on them
================
*/
void Map_Free()
{
	map::PointFile::Instance().clear();

	GlobalShaderClipboard().clear();

  g_map.m_resource->detach(g_map);
  GlobalReferenceCache().release(g_map.m_name.c_str());

	// Reset the resource pointer
	g_map.m_resource = ReferenceCache::ResourcePtr();

  FlushReferences();

}

/* greebo: Finds an entity with the given classname
 */
Entity* Scene_FindEntityByClass(const std::string& className) {
	// Instantiate a walker to find the entity
	EntityFindByClassnameWalker walker(className);
	
	// Walk the scenegraph
	GlobalSceneGraph().traverse(walker);
	
	return walker.getEntity();
}

void Map_RemoveSavedPosition() {
	const std::string keyLastCamPos = GlobalRegistry().get(RKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = GlobalRegistry().get(RKEY_LAST_CAM_ANGLE);
	
	Entity* worldspawn = Scene_FindEntityByClass("worldspawn");
	
	if (worldspawn != NULL) {
		worldspawn->setKeyValue(keyLastCamPos, "");
		worldspawn->setKeyValue(keyLastCamAngle, "");
	}
}

/* greebo: Saves the current camera position/angles to worldspawn
 */
void Map_SavePosition() {
	const std::string keyLastCamPos = GlobalRegistry().get(RKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = GlobalRegistry().get(RKEY_LAST_CAM_ANGLE); 
	
	Entity* worldspawn = Scene_FindEntityByClass("worldspawn");
	
	if (worldspawn != NULL) {
		CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
				
		worldspawn->setKeyValue(keyLastCamPos, camwnd.getCameraOrigin());
		worldspawn->setKeyValue(keyLastCamAngle, camwnd.getCameraAngles());
	}
}

/* Find the start position in the map and focus the viewport on it.
 */
void Map_StartPosition()
{
	const std::string keyLastCamPos = GlobalRegistry().get(RKEY_LAST_CAM_POSITION);
	const std::string keyLastCamAngle = GlobalRegistry().get(RKEY_LAST_CAM_ANGLE); 
	const std::string eClassPlayerStart = GlobalRegistry().get(RKEY_PLAYER_START_ECLASS);
	
	Vector3 angles(0,0,0);
	Vector3 origin(0,0,0);
	
	Entity* worldspawn = Scene_FindEntityByClass("worldspawn");
	
	if (worldspawn != NULL) {
		// Try to find a saved "last camera position"
		const std::string savedOrigin = worldspawn->getKeyValue(keyLastCamPos);
		
		if (savedOrigin != "") {
			// Construct the vector out of the std::string
			origin = Vector3(savedOrigin);
			
			Vector3 angles = worldspawn->getKeyValue(keyLastCamAngle);
			
			// Focus the view with the default angle
			map::focusViews(origin, angles);
			
			// Remove the saved entity key value so it doesn't appear during map edit
			Map_RemoveSavedPosition();
			
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
	map::focusViews(origin, angles);
}

/* Check if a node is the worldspawn.
 */
inline bool node_is_worldspawn(scene::INodePtr node) {
	Entity* entity = Node_getEntity(node);
	return entity != 0 && entity->getKeyValue("classname") == "worldspawn";
}

// use first worldspawn
class entity_updateworldspawn : public scene::Traversable::Walker
{
public:
  bool pre(scene::INodePtr node) const
  {
    if(node_is_worldspawn(node))
    {
      if(Map_GetWorldspawn(g_map) == 0)
      {
        Map_SetWorldspawn(g_map, node);
      }
    }
    return false;
  }
};

scene::INodePtr Map_FindWorldspawn(Map& map)
{
  Map_SetWorldspawn(map, scene::INodePtr());

  Node_getTraversable(GlobalSceneGraph().root())->traverse(entity_updateworldspawn());

  return Map_GetWorldspawn(map);
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

void Map_UpdateWorldspawn(Map& map)
{
  if(Map_FindWorldspawn(map) == 0)
  {
    Map_SetWorldspawn(map, createWorldspawn());
  }
}

scene::INodePtr Map_FindOrInsertWorldspawn(Map& map)
{
  Map_UpdateWorldspawn(map);
  return Map_GetWorldspawn(map);
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
			scene::INodePtr world_node = Map_FindWorldspawn(g_map);
			
			if (world_node == NULL) {
				// Set the worldspawn to the new node
				Map_SetWorldspawn(g_map, node);
				
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


typedef std::map<std::string, std::size_t> EntityBreakdown;

class EntityBreakdownWalker : public scene::Graph::Walker
{
  EntityBreakdown& m_entitymap;
public:
  EntityBreakdownWalker(EntityBreakdown& entitymap)
    : m_entitymap(entitymap)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    Entity* entity = Node_getEntity(path.top());
    if(entity != 0)
    {
      IEntityClassConstPtr eclass = entity->getEntityClass();
      std::string ecName = eclass->getName();
      
      if(m_entitymap.find(ecName) == m_entitymap.end())
      {
        m_entitymap[ecName] = 1;
      }
      else ++m_entitymap[ecName];
    }
    return true;
  }
};

void Scene_EntityBreakdown(EntityBreakdown& entitymap)
{
  GlobalSceneGraph().traverse(EntityBreakdownWalker(entitymap));
}

void DoMapInfo()
{
  ModalDialog dialog;
  GtkEntry* brushes_entry;
  GtkEntry* entities_entry;
  GtkListStore* EntityBreakdownWalker;

  GtkWindow* window = create_dialog_window(MainFrame_getWindow(), "Map Info", G_CALLBACK(dialog_delete_callback), &dialog);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

  {
    GtkVBox* vbox = create_dialog_vbox(4, 4);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));

    {
      GtkHBox* hbox = create_dialog_hbox(4);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), FALSE, TRUE, 0);

      {
        GtkTable* table = create_dialog_table(2, 2, 4, 4);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(table), TRUE, TRUE, 0);

        {
          GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
          gtk_widget_show(GTK_WIDGET(entry));
          gtk_table_attach(table, GTK_WIDGET(entry), 1, 2, 0, 1,
                            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_entry_set_editable(entry, FALSE);

          brushes_entry = entry;
        }
        {
          GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
          gtk_widget_show(GTK_WIDGET(entry));
          gtk_table_attach(table, GTK_WIDGET(entry), 1, 2, 1, 2,
                            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_entry_set_editable(entry, FALSE);

          entities_entry = entry;
        }
        {
          GtkWidget* label = gtk_label_new ("Total Brushes");
          gtk_widget_show (label);
          gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        }
        {
          GtkWidget* label = gtk_label_new ("Total Entities");
          gtk_widget_show (label);
          gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
          gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        }
      }
      {
        GtkVBox* vbox2 = create_dialog_vbox(4);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox2), FALSE, FALSE, 0);

        {
          GtkButton* button = create_dialog_button("Close", G_CALLBACK(dialog_button_ok), &dialog);
          gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(button), FALSE, FALSE, 0);
        }
      }
    }
    {
      GtkWidget* label = gtk_label_new ("Entity breakdown");
      gtk_widget_show (label);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(label), FALSE, TRUE, 0);
      gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    }
    {
      GtkScrolledWindow* scr = create_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC, 4);
      gtk_box_pack_start(GTK_BOX (vbox), GTK_WIDGET(scr), TRUE, TRUE, 0);

      {
        GtkListStore* store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

        GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
        gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(view), TRUE);

        {
          GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
          GtkTreeViewColumn* column = 
          	gtk_tree_view_column_new_with_attributes("Entity", 
          											 renderer, 
          											 "text", 0, 
          											 NULL);
          gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
          gtk_tree_view_column_set_sort_column_id(column, 0);
        }

        {
          GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
          GtkTreeViewColumn* column = 
          	gtk_tree_view_column_new_with_attributes("Count", 
          											 renderer, 
          											 "text", 1, 
          											 NULL);
          gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
          gtk_tree_view_column_set_sort_column_id(column, 1);
        }

        gtk_widget_show(view);

        gtk_container_add(GTK_CONTAINER(scr), view);
        
        EntityBreakdownWalker = store;
      }
    }
  }

  // Initialize fields

  {
    EntityBreakdown entitymap;
    Scene_EntityBreakdown(entitymap);

    for(EntityBreakdown::iterator i=entitymap.begin(); i != entitymap.end(); ++i)
    {
      char tmp[16];
      sprintf (tmp, "%u", Unsigned((*i).second));
      GtkTreeIter iter;
      gtk_list_store_append(GTK_LIST_STORE(EntityBreakdownWalker), &iter);
      gtk_list_store_set(GTK_LIST_STORE(EntityBreakdownWalker), &iter, 0, (*i).first.c_str(), 1, tmp, -1);
    }
  }

  g_object_unref(G_OBJECT(EntityBreakdownWalker));

  char tmp[16];
  sprintf (tmp, "%u", Unsigned(GlobalRadiant().getCounter(counterBrushes).get()));
  gtk_entry_set_text (GTK_ENTRY (brushes_entry), tmp);
  sprintf (tmp, "%u", Unsigned(GlobalRadiant().getCounter(counterEntities).get()));
  gtk_entry_set_text (GTK_ENTRY (entities_entry), tmp);

  modal_dialog_show(window, dialog);

  gtk_widget_destroy(GTK_WIDGET(window));
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

/*
================
Map_LoadFile
================
*/

void Map_LoadFile (const std::string& filename)
{
  globalOutputStream() << "Loading map from " << filename << "\n";

  g_map.m_name = filename;
  Map_UpdateTitle(g_map);

  {
    ScopeTimer timer("map load");

    g_map.m_resource = GlobalReferenceCache().capture(g_map.m_name);
    g_map.m_resource->attach(g_map);

	// Get the traversable root
	scene::TraversablePtr rt = Node_getTraversable(GlobalSceneGraph().root());
	assert(rt != NULL);
	
	// Traverse the scenegraph and find the worldspawn 
    rt->traverse(entity_updateworldspawn());
  }

  globalOutputStream() << "--- LoadMapFile ---\n";
  globalOutputStream() << g_map.m_name.c_str() << "\n";
  
  globalOutputStream() << makeLeftJustified(Unsigned(GlobalRadiant().getCounter(counterBrushes).get()), 5) << " primitive\n";
  globalOutputStream() << makeLeftJustified(Unsigned(GlobalRadiant().getCounter(counterEntities).get()), 5) << " entities\n";

	// Add the origin to all the children of func_static, etc.
	map::addOriginToChildPrimitives();

	// Move the view to a start position
	Map_StartPosition();

	// Load the stored map positions from the worldspawn entity
	map::GlobalMapPosition().loadPositions();
	// Remove them, so that the user doesn't get bothered with them
	map::GlobalMapPosition().removePositions();
	
	// Disable the region to make sure
	GlobalRegion().disable();
	
	// Clear the shaderclipboard, the references are most probably invalid now
	GlobalShaderClipboard().clear();
	
	// Clear the modified flag
	map::setModified(false);
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

  g_map.m_resource->detach(g_map);
  GlobalReferenceCache().release(g_map.m_name.c_str());

	g_map.m_resource = resource;

  g_map.m_name = absolute;
  Map_UpdateTitle(g_map);

  g_map.m_resource->attach(g_map);
}

void Map_Rename(const std::string& filename)
{
	if(g_map.m_name != filename) {
    	ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Saving Map");

    	Map_RenameAbsolute(filename.c_str());
    
    	SceneChangeNotify();
	}
	else {
    	SaveReferences();
  	}
}

void Map_Save()
{
	// Store the camview position into worldspawn
	Map_SavePosition();
	
	// Store the map positions into the worldspawn spawnargs
	map::GlobalMapPosition().savePositions();
	
	map::PointFile::Instance().clear();

	ScopeTimer timer("map save");
	
	// Substract the origin from child primitives (of entities like func_static)
	map::removeOriginFromChildPrimitives();
	
	// Save the actual map, by iterating through the reference cache and saving
	// each ModelResource.
	SaveReferences();
	
	// Re-add the origins to the child primitives (of entities like func_static)
	map::addOriginToChildPrimitives();
  
	// Remove the saved camera position
	Map_RemoveSavedPosition();
	
	// Remove the map positions again after saving
	map::GlobalMapPosition().removePositions();
	
	// Clear the modified flag
	map::setModified(false);
}

/*
===========
Map_New

===========
*/
void Map_New()
{
	//globalOutputStream() << "Map_New\n";

	g_map.m_name = "unnamed.map";
  Map_UpdateTitle(g_map);

  {
    g_map.m_resource = GlobalReferenceCache().capture(g_map.m_name.c_str());
//    ASSERT_MESSAGE(g_map.m_resource->getNode() == 0, "bleh");
    g_map.m_resource->attach(g_map);

    SceneChangeNotify();
  }

  map::focusViews(g_vector3_identity, Vector3(0,0,0));

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

//
//================
//Map_ImportFile
//================
//
bool Map_ImportFile(const std::string& filename)
{
  bool success = false;
  {
	ReferenceCache::ResourcePtr resource = 
		GlobalReferenceCache().capture(filename);
		
    resource->refresh(); // avoid loading old version if map has changed on disk since last import
    if(resource->load())
    {
      scene::INodePtr clone(NewMapRoot(""));

      {
        //ScopeTimer timer("clone subgraph");
        
        // Add the origin to all the child brushes
        Node_getTraversable(resource->getNode())->traverse(map::OriginAdder());
        
        Node_getTraversable(resource->getNode())->traverse(CloneAll(clone));
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

/*
===========
Map_SaveFile
===========
*/
bool Map_SaveFile(const char* filename)
{
  ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Saving Map");
  return MapResource_saveFile(MapFormat_forFile(filename), GlobalSceneGraph().root(), Map_Traverse, filename); 
}

//
//===========
//Map_SaveSelected
//===========
//
// Saves selected world brushes and whole entities with partial/full selections
//
bool Map_SaveSelected(const std::string& filename) {
	// Substract the origin from child primitives (of entities like func_static)
	map::removeOriginFromChildPrimitives();
	
	bool success = MapResource_saveFile(MapFormat_forFile(filename), 
  							  GlobalSceneGraph().root(), 
  							  Map_Traverse_Selected, 
  							  filename.c_str());

	// Add the origin to all the children of func_static, etc.
	map::addOriginToChildPrimitives();

	return success;
}

#include "map/ParentSelectedPrimitivesToEntityWalker.h"

void Scene_parentSelectedPrimitivesToEntity(scene::Graph& graph, scene::INodePtr parent)
{
  graph.traverse(ParentSelectedPrimitivesToEntityWalker(parent));
}



namespace map {

	namespace {

		/* Walker class to subtract a Vector3 origin from each selected brush
		 * that it visits.
		 */
	
		class PrimitiveOriginSubtractor
		: public scene::Graph::Walker
		{
			// The translation matrix from the vector3
			Matrix4 _transMat;
			
		public:
		
			// Constructor
			PrimitiveOriginSubtractor(const Vector3& origin)
			: _transMat(Matrix4::getTranslation( origin*(-1) )) {}
			
			// Pre visit function
			bool pre(const scene::Path& path, scene::Instance& instance) const {
				if (Node_isPrimitive(path.top())) {
					Selectable* selectable = Instance_getSelectable(instance);
      				if(selectable != 0 && selectable->isSelected() && path.size() > 1) {
						return false;
					}
      			}
    			return true;
  			}
				
			// Post visit function
			void post(const scene::Path& path, scene::Instance& instance) const {
				if (Node_isPrimitive(path.top())) {
					Selectable* selectable = Instance_getSelectable(instance);
					if(selectable != 0 && selectable->isSelected() && path.size() > 1) {
						// Node is selected, check if it is a brush.
						Brush* brush = Node_getBrush(path.top());
						if (brush != 0) {
							// We have a brush, apply the transformation
							brush->transform(_transMat);
							brush->freezeTransform();
						}	
					}
				}	
			} // post()

		}; // BrushOriginSubtractor
		 

		/** Walker class to count the number of selected brushes in the current
		 * scene.
		 */
	
		class CountSelectedPrimitives : public scene::Graph::Walker
		{
		  int& m_count;
		  mutable std::size_t m_depth;
		public:
		  CountSelectedPrimitives(int& count) : m_count(count), m_depth(0)
		  {
		    m_count = 0;
		  }
		  bool pre(const scene::Path& path, scene::Instance& instance) const
		  {
		    if(++m_depth != 1 && path.top()->isRoot())
		    {
		      return false;
		    }
		    Selectable* selectable = Instance_getSelectable(instance);
		    if(selectable != 0
		      && selectable->isSelected()
		      && Node_isPrimitive(path.top()))
		    {
		      ++m_count;
		    }
		    return true;
		  }
		  void post(const scene::Path& path, scene::Instance& instance) const
		  {
		    --m_depth;
		  }
		};
		
		/** greebo: Counts the selected brushes in the scenegraph
		 */
		class BrushCounter : public scene::Graph::Walker
		{
			int& _count;
			mutable std::size_t _depth;
		public:
			BrushCounter(int& count) : 
				_count(count), 
				_depth(0) 
			{
				_count = 0;
			}
			
			bool pre(const scene::Path& path, scene::Instance& instance) const {
				
				if (++_depth != 1 && path.top()->isRoot()) {
					return false;
				}
				
				Selectable* selectable = Instance_getSelectable(instance);
				if (selectable != NULL && selectable->isSelected()
				        && Node_isBrush(path.top())) 
				{
					++_count;
				}
				
				return true;
			}
			
			void post(const scene::Path& path, scene::Instance& instance) const {
				--_depth;
			}
		};

	} // namespace

	/* Subtract the given origin from all selected primitives in the map. Uses
	 * a PrimitiveOriginSubtractor walker class to subtract the origin from
	 * each selected primitive in the scene.
	 */
	 
	void selectedPrimitivesSubtractOrigin(const Vector3& origin) {
		GlobalSceneGraph().traverse(PrimitiveOriginSubtractor(origin));
	}	
	
	/* Return the number of selected primitives in the map, using the
	 * CountSelectedPrimitives walker.
	 */
	int countSelectedPrimitives() {
		int count;
		GlobalSceneGraph().traverse(CountSelectedPrimitives(count));
		return count;
	}
	
	/* Return the number of selected brushes in the map, using the
	 * CountSelectedBrushes walker.
	 */
	int countSelectedBrushes() {
		int count;
		GlobalSceneGraph().traverse(BrushCounter(count));
		return count;
	}

} // namespace map

// Moved from qe3.cpp to here
bool ConfirmModified(const char* title) {
  if (!Map_Modified(g_map))
    return true;

  EMessageBoxReturn result = gtk_MessageBox(GTK_WIDGET(MainFrame_getWindow()), "The current map has changed since it was last saved.\nDo you want to save the current map before continuing?", title, eMB_YESNOCANCEL, eMB_ICONQUESTION);
  if(result == eIDCANCEL)
  {
    return false;
  }
  if(result == eIDYES)
  {
    if(Map_Unnamed(g_map))
    {
      return Map_SaveAs();
    }
    else
    {
      Map_Save();
    }
  }
  return true;
}

void NewMap() {
	if (ConfirmModified("New Map")) {
		// Turn regioning off when starting a new map
		GlobalRegion().disable();

		Map_Free();
		Map_New();
	}
}

void OpenMap()
{
  if (!ConfirmModified("Open Map"))
    return;

	// Get the map file name to load
	std::string filename = map::MapFileManager::getMapFilename(true, 
															   "Open map");

	if (!filename.empty()) {
	    GlobalMRU().insert(filename);
	    Map_Free();
	    Map_LoadFile(filename);
	}
}

void ImportMap()
{
	std::string filename = map::MapFileManager::getMapFilename(true,
															   "Import map");

	if (!filename.empty()) {
	    UndoableCommand undo("mapImport");
	    Map_ImportFile(filename);
	}
}

namespace map {

void loadPrefabAt(const Vector3& targetCoords) {
	std::string filename = map::MapFileManager::getMapFilename(true, "Load Prefab", "prefab");
	
	if (!filename.empty()) {
		UndoableCommand undo("loadPrefabAt");
	    
	    // Deselect everything
	    GlobalSelectionSystem().setSelectedAll(false);
	    
	    // Now import the prefab (imported items get selected)
	    Map_ImportFile(filename);
	    
	    // Translate the selection to the given point
	    GlobalSelectionSystem().translateSelected(targetCoords);
	}
}

void loadPrefab() {
	loadPrefabAt(Vector3(0,0,0));
}

void saveSelectedAsPrefab() {
	std::string filename = 
		map::MapFileManager::getMapFilename(false, "Save selected as Prefab", "prefab");
	
	if (!filename.empty()) {
	    Map_SaveSelected(filename);
  	}
}

} // namespace map

bool Map_SaveAs()
{
	std::string filename = map::MapFileManager::getMapFilename(false, "Save map");
  
	if (!filename.empty()) {
	    GlobalMRU().insert(filename);
	    Map_Rename(filename);
	    Map_Save();
	    return true;
	}
	else {
		return false;
	}
}

void SaveMapAs()
{
  Map_SaveAs();
}

void SaveMap()
{
  if(Map_Unnamed(g_map))
  {
    SaveMapAs();
  }
  else if(Map_Modified(g_map))
  {
    Map_Save();
  }
}

void ExportMap()
{
	std::string filename = map::MapFileManager::getMapFilename(
								false, "Export selection");

	if (!filename.empty()) {
	    Map_SaveSelected(filename);
  	}
}

class BrushFindByIndexWalker : public scene::Traversable::Walker
{
  mutable std::size_t m_index;
  scene::Path& m_path;
public:
  BrushFindByIndexWalker(std::size_t index, scene::Path& path)
    : m_index(index), m_path(path)
  {
  }
  bool pre(scene::INodePtr node) const
  {
    if(Node_isPrimitive(node) && m_index-- == 0)
    {
      m_path.push(node);
    }
    return false;
  }
};

class EntityFindByIndexWalker : public scene::Traversable::Walker
{
  mutable std::size_t m_index;
  scene::Path& m_path;
public:
  EntityFindByIndexWalker(std::size_t index, scene::Path& path)
    : m_index(index), m_path(path)
  {
  }
  bool pre(scene::INodePtr node) const
  {
    if(Node_isEntity(node) && m_index-- == 0)
    {
      m_path.push(node);
    }
    return false;
  }
};

void Scene_FindEntityBrush(std::size_t entity, std::size_t brush, scene::Path& path)
{
  path.push(GlobalSceneGraph().root());
  {
    Node_getTraversable(path.top())->traverse(EntityFindByIndexWalker(entity, path));
  }
  if(path.size() == 2)
  {
    scene::TraversablePtr traversable = Node_getTraversable(path.top());
    if(traversable != 0)
    {
      traversable->traverse(BrushFindByIndexWalker(brush, path));
    }
  }
}

inline bool Node_hasChildren(scene::INodePtr node)
{
  scene::TraversablePtr traversable = Node_getTraversable(node);
  return traversable != NULL && !traversable->empty();
}

void SelectBrush (int entitynum, int brushnum)
{
  scene::Path path;
  Scene_FindEntityBrush(entitynum, brushnum, path);
  if(path.size() == 3 || (path.size() == 2 && !Node_hasChildren(path.top())))
  {
    scene::Instance* instance = GlobalSceneGraph().find(path);
    ASSERT_MESSAGE(instance != 0, "SelectBrush: path not found in scenegraph");
    Selectable* selectable = Instance_getSelectable(*instance);
    ASSERT_MESSAGE(selectable != 0, "SelectBrush: path not selectable");
    selectable->setSelected(true);
    
    XYWnd* xyView = GlobalXYWnd().getActiveXY();
    
    if (xyView != NULL) {
    	xyView->positionView(instance->worldAABB().origin);
    }
  }
}


class BrushFindIndexWalker : public scene::Graph::Walker
{
  mutable scene::INodePtr m_node;
  std::size_t& m_count;
public:
  BrushFindIndexWalker(const scene::INodePtr node, std::size_t& count)
    : m_node(node), m_count(count)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    if(Node_isPrimitive(path.top()))
    {
      if(m_node == path.top()) {
        m_node = scene::INodePtr();
      }
      if(m_node)
      {
        ++m_count;
      }
    }
    return true;
  }
};

class EntityFindIndexWalker : public scene::Graph::Walker
{
  mutable scene::INodePtr m_node;
  std::size_t& m_count;
public:
  EntityFindIndexWalker(const scene::INodePtr node, std::size_t& count)
    : m_node(node), m_count(count)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    if(Node_isEntity(path.top()))
    {
      if(m_node == path.top())
      {
        m_node = scene::INodePtr();
      }
      if(m_node)
      {
        ++m_count;
      }
    }
    return true;
  }
};

static void GetSelectionIndex (int *ent, int *brush)
{
  std::size_t count_brush = 0;
  std::size_t count_entity = 0;
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    const scene::Path& path = GlobalSelectionSystem().ultimateSelected().path();

    GlobalSceneGraph().traverse(BrushFindIndexWalker(path.top(), count_brush));
    GlobalSceneGraph().traverse(EntityFindIndexWalker(path.parent(), count_entity));
  }
  *brush = int(count_brush);
  *ent = int(count_entity);
}

void DoFind()
{
  ModalDialog dialog;
  GtkEntry* entity;
  GtkEntry* brush;

  GtkWindow* window = create_dialog_window(MainFrame_getWindow(), "Find Brush", G_CALLBACK(dialog_delete_callback), &dialog);

  GtkAccelGroup* accel = gtk_accel_group_new();
  gtk_window_add_accel_group(window, accel);

  {
    GtkVBox* vbox = create_dialog_vbox(4, 4);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));
    {
      GtkTable* table = create_dialog_table(2, 2, 4, 4);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(table), TRUE, TRUE, 0);
      {
        GtkWidget* label = gtk_label_new ("Entity number");
        gtk_widget_show (label);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                          (GtkAttachOptions) (0),
                          (GtkAttachOptions) (0), 0, 0);
      }
      {
        GtkWidget* label = gtk_label_new ("Brush number");
        gtk_widget_show (label);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                          (GtkAttachOptions) (0),
                          (GtkAttachOptions) (0), 0, 0);
      }
      {
        GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
        gtk_widget_show(GTK_WIDGET(entry));
        gtk_table_attach(table, GTK_WIDGET(entry), 1, 2, 0, 1,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_widget_grab_focus(GTK_WIDGET(entry));
        entity = entry;
      }
      {
        GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
        gtk_widget_show(GTK_WIDGET(entry));
        gtk_table_attach(table, GTK_WIDGET(entry), 1, 2, 1, 2,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);

        brush = entry;
      }
    }
    {
      GtkHBox* hbox = create_dialog_hbox(4);
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), TRUE, TRUE, 0);
      {
        GtkButton* button = create_dialog_button("Find", G_CALLBACK(dialog_button_ok), &dialog);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        widget_make_default(GTK_WIDGET(button));
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Return, (GdkModifierType)0, (GtkAccelFlags)0);
      }
      {
        GtkButton* button = create_dialog_button("Close", G_CALLBACK(dialog_button_cancel), &dialog);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0);
      }
    }
  }

  // Initialize dialog
  char buf[16];
  int ent, br;

  GetSelectionIndex (&ent, &br);
  sprintf (buf, "%i", ent);
  gtk_entry_set_text(entity, buf);
  sprintf (buf, "%i", br);
  gtk_entry_set_text(brush, buf);

  if(modal_dialog_show(window, dialog) == eIDOK)
  {
    const char *entstr = gtk_entry_get_text(entity);
    const char *brushstr = gtk_entry_get_text(brush);
    SelectBrush (atoi(entstr), atoi(brushstr));
  }

  gtk_widget_destroy(GTK_WIDGET(window));
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
      if(g_map.m_resource != 0)
      {
	      g_map.m_resource->realise();
      }
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
      if(g_map.m_resource != 0)
      {
        g_map.m_resource->flush();
	      g_map.m_resource->unrealise();
      }
    }
  }
};

MapEntityClasses g_MapEntityClasses;

#include "preferencesystem.h"

void Map_Construct()
{
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
