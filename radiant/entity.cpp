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

#include "entity.h"

#include "ieventmanager.h"
#include "ientity.h"
#include "ieclass.h"
#include "iselection.h"
#include "imodel.h"
#include "ifilesystem.h"
#include "iundo.h"
#include "editable.h"

#include "scenelib.h"
#include "os/path.h"
#include "os/file.h"

#include "gtkutil/filechooser.h"
#include "gtkmisc.h"
#include "select.h"
#include "map.h"
#include "preferences.h"
#include "gtkdlgs.h"
#include "mainframe.h"
#include "qe3.h"

#include "selection/algorithm/Shader.h"
#include "ui/modelselector/ModelSelector.h"

#include <iostream>

	namespace {
		const std::string RKEY_FREE_MODEL_ROTATION = "user/ui/freeModelRotation";
	}

class EntitySetKeyValueSelected : public scene::Graph::Walker
{
  const char* m_key;
  const char* m_value;
public:
  EntitySetKeyValueSelected(const char* key, const char* value)
    : m_key(key), m_value(value)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    return true;
  }
  void post(const scene::Path& path, scene::Instance& instance) const
  {
    Entity* entity = Node_getEntity(path.top());
    if(entity != 0
      && (instance.childSelected() || Instance_getSelectable(instance)->isSelected()))
    {
      entity->setKeyValue(m_key, m_value);
    }
  }
};

class EntitySetClassnameSelected : public scene::Graph::Walker
{
  const char* m_classname;
public:
  EntitySetClassnameSelected(const char* classname)
    : m_classname(classname)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    return true;
  }
  void post(const scene::Path& path, scene::Instance& instance) const
  {
    Entity* entity = Node_getEntity(path.top());
    if(entity != 0
      && (instance.childSelected() || Instance_getSelectable(instance)->isSelected()))
    { 
      NodeSmartReference node(GlobalEntityCreator().createEntity(GlobalEntityClassManager().findOrInsert(m_classname, node_is_group(path.top()))));

      EntityCopyingVisitor visitor(*Node_getEntity(node));

      entity->forEachKeyValue(visitor);

      NodeSmartReference child(path.top().get());
      NodeSmartReference parent(path.parent().get());
      Node_getTraversable(parent)->erase(child);
      if(Node_getTraversable(child) != 0
        && Node_getTraversable(node) != 0
        && node_is_group(node))
      {
        parentBrushes(child, node);
      }
      Node_getTraversable(parent)->insert(node);
    }
  }
};

void Scene_EntitySetKeyValue_Selected(const char* key, const char* value)
{
  GlobalSceneGraph().traverse(EntitySetKeyValueSelected(key, value));
}

void Scene_EntitySetClassname_Selected(const char* classname)
{
  GlobalSceneGraph().traverse(EntitySetClassnameSelected(classname));
}


class EntityUngroupVisitor : public SelectionSystem::Visitor
{
  const scene::Path& m_parent;
public:
  EntityUngroupVisitor(const scene::Path& parent) : m_parent(parent)
  {
  }
  void visit(scene::Instance& instance) const
  {
    if(Node_getEntity(instance.path().top()) != 0
      && node_is_group(instance.path().top()))
    {
      if(m_parent.top().get_pointer() != instance.path().top().get_pointer())
      {
        parentBrushes(instance.path().top(), m_parent.top());
        Path_deleteTop(instance.path());
      }
    }
  }
};

void Entity_ungroupSelected()
{
  UndoableCommand undo("ungroupSelectedEntities");

  scene::Path world_path(makeReference(GlobalSceneGraph().root()));
  world_path.push(makeReference(Map_FindOrInsertWorldspawn(g_map)));

  GlobalSelectionSystem().foreachSelected(EntityUngroupVisitor(world_path));
}



void Entity_connectSelected()
{
  if(GlobalSelectionSystem().countSelected() == 2)
  {
    GlobalEntityCreator().connectEntities(
      GlobalSelectionSystem().penultimateSelected().path(),
      GlobalSelectionSystem().ultimateSelected().path()
    );
  }
  else
  {
    globalErrorStream() << "entityConnectSelected: exactly two instances must be selected\n";
  }
}

// Function to return an AABB based on the current workzone AABB (retrieved
// from the currently selected brushes), or to use the default light radius
// if the workzone AABB is not valid or none is available.

AABB Doom3Light_getBounds(AABB aabb)
{
	// If the extents are 0 or invalid (-1), replace with the default radius
    for (int i = 0; i < 3; i++) {
		if (aabb.extents[i] <= 0)
	    	aabb.extents[i] = DEFAULT_LIGHT_RADIUS;
    }
	return aabb;
}

/** Create an instance of the given entity at the given position, and return
 * the Node containing the new entity.
 * 
 * @returns
 * A NodeSmartReference containing the new entity.
 */

NodeSmartReference Entity_createFromSelection(const char* name, 
											  const Vector3& origin) 
{

    IEntityClassPtr entityClass = GlobalEntityClassManager().findOrInsert(name, 
    																	  true);

    // TODO: to be replaced by inheritance-based class detection
    bool isModel = (GlobalSelectionSystem().countSelected() == 0 
                    && string_equal_nocase(name, "func_static"));
    
    // Some entities are based on the size of the currently-selected primitive(s)
    bool primitivesSelected = map::countSelectedPrimitives() != 0;

    if (!(entityClass->isFixedSize() || isModel) && !primitivesSelected) {
		throw EntityCreationException(std::string("Unable to create entity \"") 
									  + name 
									  + "\", no brushes selected");
    }

    AABB workzone(AABB::createFromMinMax(Select_getWorkZone().d_work_min, 
    									 Select_getWorkZone().d_work_max));
    
    NodeSmartReference node(GlobalEntityCreator().createEntity(entityClass));
    
    Node_getTraversable(GlobalSceneGraph().root())->insert(node);
    
    scene::Path entitypath(makeReference(GlobalSceneGraph().root()));
    entitypath.push(makeReference(node.get()));
    scene::Instance & instance = findInstance(entitypath);

    if (entityClass->isFixedSize() || (isModel && !primitivesSelected)) {
        Select_Delete();
    
        Transformable *transform = Instance_getTransformable(instance);
    
        if (transform != 0) {
            transform->setType(TRANSFORM_PRIMITIVE);
            transform->setTranslation(origin);
            transform->freezeTransform();
        }
    
        GlobalSelectionSystem().setSelectedAll(false);
    
        Instance_setSelected(instance, true);
    }
    else { // brush-based entity
    	
    	Entity* entity = Node_getEntity(node);
    	
    	// Add selected brushes as children of non-fixed entity
		entity->setKeyValue("model", Node_getEntity(node)->getKeyValue("name"));
		entity->setKeyValue("origin", std::string(workzone.getOrigin()));
		
        // If there is an "editor_material" class attribute, apply this shader
        // to all of the selected primitives before parenting them
        std::string material =
            entity->getEntityClass()->getValueForKey("editor_material");
        if (!material.empty()) {
            selection::algorithm::applyShaderToSelection(material);
        }
                
        // Parent the selected primitives
	    Scene_parentSelectedPrimitivesToEntity(GlobalSceneGraph(), node);
	    Scene_forEachChildSelectable(SelectableSetSelected(true), instance.path());
	    
	    // De-select the children and select the newly created parent entity
	    GlobalSelectionSystem().setSelectedAll(false);
	    Instance_setSelected(instance, true);
    }
	
    // Set the light radius and origin

    if (entityClass->isLight() && primitivesSelected) {
        AABB bounds(Doom3Light_getBounds(workzone));    
        Node_getEntity(node)->setKeyValue("origin", bounds.getOrigin());
        Node_getEntity(node)->setKeyValue("light_radius", bounds.getExtents());
    }
    
    // Flag the map as unsaved after creating the entity
    map::setModified(true);
    
	// Return the new node
	return node;
}

#include "preferencesystem.h"

void Entity_Construct() {
	GlobalEventManager().addCommand("ConnectSelection", FreeCaller<Entity_connectSelected>());
	GlobalEventManager().addCommand("UngroupSelection", FreeCaller<Entity_ungroupSelected>());
	GlobalEventManager().addRegistryToggle("ToggleFreeModelRotation", RKEY_FREE_MODEL_ROTATION);
}

void Entity_Destroy()
{
}

