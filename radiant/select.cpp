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

#include "select.h"

#include "debugging/debugging.h"

#include "ientity.h"
#include "iselection.h"
#include "iundo.h"

#include <vector>

#include "stream/stringstream.h"
#include "signal/isignal.h"
#include "shaderlib.h"
#include "scenelib.h"

#include "gtkutil/idledraw.h"
#include "gtkutil/dialog.h"
#include "gtkutil/widget.h"
#include "brushmanip.h"
#include "patchmanip.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "gtkmisc.h"
#include "mainframe.h"
#include "igrid.h"
#include "selection/SceneWalkers.h"
#include "brush/BrushInstance.h"
#include "xyview/GlobalXYWnd.h"

select_workzone_t g_select_workzone;


/**
  Loops over all selected brushes and stores their
  world AABBs in the specified array.
*/
class CollectSelectedBrushesBounds : public SelectionSystem::Visitor
{
  AABB* m_bounds;   // array of AABBs
  Unsigned m_max;   // max AABB-elements in array
  Unsigned& m_count;// count of valid AABBs stored in array

public:
  CollectSelectedBrushesBounds(AABB* bounds, Unsigned max, Unsigned& count)
    : m_bounds(bounds),
      m_max(max),
      m_count(count)
  {
    m_count = 0;
  }

  void visit(scene::Instance& instance) const
  {
    ASSERT_MESSAGE(m_count <= m_max, "Invalid m_count in CollectSelectedBrushesBounds");

    // stop if the array is already full
    if(m_count == m_max)
      return;

    Selectable* selectable = Instance_getSelectable(instance);
    if((selectable != 0)
      && instance.isSelected())
    {
      // brushes only
      if(Instance_getBrush(instance) != 0)
      {
        m_bounds[m_count] = instance.worldAABB();
        ++m_count;
      }
    }
  }
};

/**
  Selects all objects that intersect one of the bounding AABBs.
  The exact intersection-method is specified through TSelectionPolicy
*/
template<class TSelectionPolicy>
class SelectByBounds : public scene::Graph::Walker
{
  AABB* m_aabbs;           // selection aabbs
  Unsigned m_count;        // number of aabbs in m_aabbs
  TSelectionPolicy policy; // type that contains a custom intersection method aabb<->aabb

public:
  SelectByBounds(AABB* aabbs, Unsigned count)
      : m_aabbs(aabbs),
        m_count(count)
  {
  }

  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    Selectable* selectable = Instance_getSelectable(instance);

    // ignore worldspawn
    Entity* entity = Node_getEntity(path.top());
    if(entity)
    {
      if(entity->getKeyValue("classname") == "worldspawn")
        return true;
    }
    
    if( (path.size() > 1) &&
        (!path.top()->isRoot()) &&
        (selectable != 0)
       )
    {
      for(Unsigned i = 0; i < m_count; ++i)
      {
        if(policy.Evaluate(m_aabbs[i], instance))
        {
          selectable->setSelected(true);
        }
      }
    }

    return true;
  }

  /**
    Performs selection operation on the global scenegraph.
    If delete_bounds_src is true, then the objects which were
    used as source for the selection aabbs will be deleted.
*/
  static void DoSelection(bool delete_bounds_src = true)
  {
    if(GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive)
    {
      // we may not need all AABBs since not all selected objects have to be brushes
      const Unsigned max = (Unsigned)GlobalSelectionSystem().countSelected();
      AABB* aabbs = new AABB[max];
            
      Unsigned count;
      CollectSelectedBrushesBounds collector(aabbs, max, count);
      GlobalSelectionSystem().foreachSelected(collector);

      // nothing usable in selection
      if(!count)
      {
        delete[] aabbs;
        return;
      }
      
      // delete selected objects
      if(delete_bounds_src)// see deleteSelection
      {
        UndoableCommand undo("deleteSelected");
        Select_Delete();
      }

      // select objects with bounds
      GlobalSceneGraph().traverse(SelectByBounds<TSelectionPolicy>(aabbs, count));
      
      SceneChangeNotify();
      delete[] aabbs;
    }
  }
};

/**
  SelectionPolicy for SelectByBounds
  Returns true if 
*/
class SelectionPolicy_Complete_Tall
{
public:
	bool Evaluate(const AABB& box, scene::Instance& instance) const {
		
		// Get the AABB of the visited instance
		AABB other = instance.worldAABB();
		
		// greebo: Perform a special selection test for lights
		// as the small diamond should be tested against selection only 
		scene::LightInstance* light = Instance_getLight(instance);
		if (light != NULL) {
			other = light->getSelectAABB();
		}
		
		// Determine the viewtype
		EViewType viewType = GlobalXYWnd().getActiveViewType();
		
		unsigned int axis1 = 0;
		unsigned int axis2 = 1;
		
		// Determine which axes have to be compared
		switch (viewType) {
			case XY:
				axis1 = 0;
				axis2 = 1;
			break;
			case YZ:
				axis1 = 1;
				axis2 = 2;
			break;
			case XZ:
				axis1 = 0;
				axis2 = 2;
			break;
		};
		
		// Check if the AABB is contained 
		float dist1 = fabs(other.origin[axis1] - box.origin[axis1]) + fabs(other.extents[axis1]);
		float dist2 = fabs(other.origin[axis2] - box.origin[axis2]) + fabs(other.extents[axis2]);
		
		return (dist1 < fabs(box.extents[axis1]) && dist2 < fabs(box.extents[axis2]));
	}
};


/**
  SelectionPolicy for SelectByBounds
  Returns true if box and the AABB of instance intersect
*/
class SelectionPolicy_Touching
{
public:
  bool Evaluate(const AABB& box, scene::Instance& instance) const
  {
    const AABB& other(instance.worldAABB());
    for(Unsigned i = 0; i < 3; ++i)
    {
      if(fabsf(box.origin[i] - other.origin[i]) > (box.extents[i] + other.extents[i]))
        return false;
    }
    return true;
  }
};

/**
  SelectionPolicy for SelectByBounds
  Returns true if the AABB of instance is inside box
*/
class SelectionPolicy_Inside
{
public:
  bool Evaluate(const AABB& box, scene::Instance& instance) const
  {
    AABB other = instance.worldAABB();
    
    // greebo: Perform a special selection test for lights
	// as the small diamond should be tested against selection only 
	scene::LightInstance* light = Instance_getLight(instance);
	if (light != NULL) {
		other = light->getSelectAABB();
	}
    
    for(Unsigned i = 0; i < 3; ++i)
    {
      if(fabsf(box.origin[i] - other.origin[i]) > (box.extents[i] - other.extents[i]))
        return false;
    }
    return true;
  }
};

class DeleteSelected : public scene::Graph::Walker
{
  mutable bool m_remove;
  mutable bool m_removedChild;
public:
  DeleteSelected()
    : m_remove(false), m_removedChild(false)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    m_removedChild = false;

    Selectable* selectable = Instance_getSelectable(instance);
    if(selectable != 0
      && selectable->isSelected()
      && path.size() > 1
      && !path.top()->isRoot())
    {
      m_remove = true;

      return false;// dont traverse into child elements
    }
    return true;
  }
  void post(const scene::Path& path, scene::Instance& instance) const
  {
    
    if(m_removedChild)
    {
      m_removedChild = false;

      // delete empty entities
      Entity* entity = Node_getEntity(path.top());
      if(entity != 0
        && path.top() != GlobalMap().findWorldspawn()
        && Node_getTraversable(path.top())->empty())
      {
        Path_deleteTop(path);
      }
    }

	// node should be removed
    if(m_remove)
    {
      if(Node_isEntity(path.parent()) != 0)
      {
        m_removedChild = true;
      }

      m_remove = false;
      Path_deleteTop(path);
    }
  }
};

void Scene_DeleteSelected(scene::Graph& graph)
{
  graph.traverse(DeleteSelected());
  SceneChangeNotify();
}

void Select_Delete (void)
{
  Scene_DeleteSelected(GlobalSceneGraph());
}

class InvertSelectionWalker : public scene::Graph::Walker
{
	SelectionSystem::EMode m_mode;
	mutable Selectable* m_selectable;
public:
	InvertSelectionWalker(SelectionSystem::EMode mode)
			: m_mode(mode), m_selectable(0)
	{}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		Selectable* selectable = Instance_getSelectable(instance);
		if (selectable) {
			switch (m_mode) {
				case SelectionSystem::eEntity:
					if (Node_isEntity(path.top()) != 0) {
						m_selectable = path.top()->visible() ? selectable : 0;
					}
					break;
				case SelectionSystem::ePrimitive:
					m_selectable = path.top()->visible() ? selectable : 0;
					break;
				case SelectionSystem::eComponent:
					// Check if we have a componentselectiontestable instance
					ComponentSelectionTestable* compSelTestable = Instance_getComponentSelectionTestable(instance);

					// Only add it to the list if the instance has components and is already selected
					if (compSelTestable && selectable->isSelected()) {
						m_selectable = path.top()->visible() ? selectable : 0;
					}
					break;
			}
		}
		
		// Do we have a groupnode? If yes, don't traverse the children
		Entity* entity = Node_getEntity(path.top());
		if (entity != NULL && node_is_group(path.top()) && 
			entity->getKeyValue("classname") != "worldspawn") 
		{
			// Don't traverse the children of this groupnode
			return false;
		}
		return true;
	}
	
	void post(const scene::Path& path, scene::Instance& instance) const {
		if (m_selectable != 0) {
			m_selectable->invertSelected();
			m_selectable = 0;
		}
	}
};


void Scene_Invert_Selection(scene::Graph& graph)
{
  graph.traverse(InvertSelectionWalker(GlobalSelectionSystem().Mode()));
}

void Select_Invert()
{
  Scene_Invert_Selection(GlobalSceneGraph());
}

class ExpandSelectionToEntitiesWalker : public scene::Graph::Walker
{
  mutable std::size_t m_depth;
public:
  ExpandSelectionToEntitiesWalker() : m_depth(0)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    ++m_depth;
    if(m_depth == 2) // entity depth
    {
      // traverse and select children if any one is selected
      return Node_getEntity(path.top())->isContainer() && instance.childSelected();
    }
    else if(m_depth == 3) // primitive depth
    {
      Instance_setSelected(instance, true);
      return false;
    }
    return true;
  }
  void post(const scene::Path& path, scene::Instance& instance) const
  {
    --m_depth;
  }
};

void Scene_ExpandSelectionToEntities()
{
  GlobalSceneGraph().traverse(ExpandSelectionToEntitiesWalker());
}


namespace
{
  void Selection_UpdateWorkzone()
  {
    if(GlobalSelectionSystem().countSelected() != 0)
    {
      Select_GetBounds(g_select_workzone.d_work_min, g_select_workzone.d_work_max);
    }
  }
  typedef FreeCaller<Selection_UpdateWorkzone> SelectionUpdateWorkzoneCaller;

  IdleDraw g_idleWorkzone = IdleDraw(SelectionUpdateWorkzoneCaller());
}

const select_workzone_t& Select_getWorkZone()
{
  g_idleWorkzone.flush();
  return g_select_workzone;
}

void UpdateWorkzone_ForSelection()
{
  g_idleWorkzone.queueDraw();
}

// update the workzone to the current selection
void UpdateWorkzone_ForSelectionChanged(const Selectable& selectable)
{
  if(selectable.isSelected())
  {
    UpdateWorkzone_ForSelection();
  }
}

void Select_SetShader(const char* shader)
{
  if(GlobalSelectionSystem().Mode() != SelectionSystem::eComponent)
  {
    Scene_BrushSetShader_Selected(GlobalSceneGraph(), shader);
    Scene_PatchSetShader_Selected(GlobalSceneGraph(), shader);
  }
  Scene_BrushSetShader_Component_Selected(GlobalSceneGraph(), shader);
}

void Select_GetBounds (Vector3& mins, Vector3& maxs)
{
  AABB bounds;
  GlobalSceneGraph().traverse(BoundsSelected(bounds));
  if (bounds.isValid()) {
  	maxs = bounds.origin + bounds.extents;
  	mins = bounds.origin - bounds.extents;
  }
  else {
  	maxs = Vector3(0,0,0);
  	mins = Vector3(0,0,0);
  }
}

void Select_GetMid (Vector3& mid)
{
  AABB bounds;
  GlobalSceneGraph().traverse(BoundsSelected(bounds));
  mid = vector3_snapped(bounds.origin);
}


void Select_FlipAxis (int axis)
{
  Vector3 flip(1, 1, 1);
  flip[axis] = -1;
  GlobalSelectionSystem().scaleSelected(flip);
}


enum axis_t
{
  eAxisX = 0,
  eAxisY = 1,
  eAxisZ = 2,
};

enum sign_t
{
  eSignPositive = 1,
  eSignNegative = -1,
};

inline Matrix4 matrix4_rotation_for_axis90(axis_t axis, sign_t sign)
{
  switch(axis)
  {
  case eAxisX:
    if(sign == eSignPositive)
    {
      return matrix4_rotation_for_sincos_x(1, 0);
    }
    else
    {
      return matrix4_rotation_for_sincos_x(-1, 0);
    }
  case eAxisY:
    if(sign == eSignPositive)
    {
      return matrix4_rotation_for_sincos_y(1, 0);
    }
    else
    {
      return matrix4_rotation_for_sincos_y(-1, 0);
    }
  default://case eAxisZ:
    if(sign == eSignPositive)
    {
      return matrix4_rotation_for_sincos_z(1, 0);
    }
    else
    {
      return matrix4_rotation_for_sincos_z(-1, 0);
    }
  }
}

inline Quaternion quaternion_for_axis90(axis_t axis, sign_t sign)
{
#if 1
  switch(axis)
  {
  case eAxisX:
    if(sign == eSignPositive)
    {
      return Quaternion(c_half_sqrt2f, 0, 0, c_half_sqrt2f);
    }
    else
    {
      return Quaternion(-c_half_sqrt2f, 0, 0, -c_half_sqrt2f);
    }
  case eAxisY:
    if(sign == eSignPositive)
    {
      return Quaternion(0, c_half_sqrt2f, 0, c_half_sqrt2f);
    }
    else
    {
      return Quaternion(0, -c_half_sqrt2f, 0, -c_half_sqrt2f);
    }
  default://case eAxisZ:
    if(sign == eSignPositive)
    {
      return Quaternion(0, 0, c_half_sqrt2f, c_half_sqrt2f);
    }
    else
    {
      return Quaternion(0, 0, -c_half_sqrt2f, -c_half_sqrt2f);
    }
  }
#else
  quaternion_for_matrix4_rotation(matrix4_rotation_for_axis90((axis_t)axis, (deg > 0) ? eSignPositive : eSignNegative));
#endif
}

void Select_RotateAxis (int axis, float deg)
{
  if(fabs(deg) == 90.f)
  {
    GlobalSelectionSystem().rotateSelected(quaternion_for_axis90((axis_t)axis, (deg > 0) ? eSignPositive : eSignNegative));
  }
  else
  {
    switch(axis)
    {
    case 0:
      GlobalSelectionSystem().rotateSelected(quaternion_for_matrix4_rotation(matrix4_rotation_for_x_degrees(deg)));
      break;
    case 1:
      GlobalSelectionSystem().rotateSelected(quaternion_for_matrix4_rotation(matrix4_rotation_for_y_degrees(deg)));
      break;
    case 2:
      GlobalSelectionSystem().rotateSelected(quaternion_for_matrix4_rotation(matrix4_rotation_for_z_degrees(deg)));
      break;
    }
  }
}

typedef std::vector<std::string> Classnames;

bool classnames_match_entity(const Classnames& classnames, Entity* entity)
{
  for(Classnames::const_iterator i = classnames.begin(); i != classnames.end(); ++i)
  {
    if(entity->getKeyValue("classname") == *i)
    {
      return true;
    }
  }
  return false;
}

class EntitySelectByClassnameWalker : public scene::Graph::Walker
{
  const Classnames& m_classnames;
public:
  EntitySelectByClassnameWalker(const Classnames& classnames)
    : m_classnames(classnames)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    Entity* entity = Node_getEntity(path.top());
    if(entity != 0
      && classnames_match_entity(m_classnames, entity))
    {
      Instance_getSelectable(instance)->setSelected(true);
    }
    return true;
  }
};

void Scene_EntitySelectByClassnames(scene::Graph& graph, const Classnames& classnames)
{
  graph.traverse(EntitySelectByClassnameWalker(classnames));
}

class EntityGetSelectedClassnamesWalker : public scene::Graph::Walker
{
  Classnames& m_classnames;
public:
  EntityGetSelectedClassnamesWalker(Classnames& classnames)
    : m_classnames(classnames)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    Selectable* selectable = Instance_getSelectable(instance);
    if(selectable != 0
      && selectable->isSelected())
    {
      Entity* entity = Node_getEntity(path.top());
      if(entity != 0)
      {
        m_classnames.push_back(entity->getKeyValue("classname"));
      }
    }
    return true;
  }
};

void Scene_EntityGetClassnames(scene::Graph& graph, Classnames& classnames)
{
  graph.traverse(EntityGetSelectedClassnamesWalker(classnames));
}

void Select_AllOfType()
{
  if(GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
  {
    if(GlobalSelectionSystem().ComponentMode() == SelectionSystem::eFace)
    {
      GlobalSelectionSystem().setSelectedAllComponents(false);
      Scene_BrushSelectByShader_Component(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
    }
  }
  else
  {
    Classnames classnames;
    Scene_EntityGetClassnames(GlobalSceneGraph(), classnames);
    GlobalSelectionSystem().setSelectedAll(false);
    if(!classnames.empty())
    {
      Scene_EntitySelectByClassnames(GlobalSceneGraph(), classnames);
    }
    else
    {
      Scene_BrushSelectByShader(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
      Scene_PatchSelectByShader(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
    }
  }
}

void Select_Inside(void)
{
	SelectByBounds<SelectionPolicy_Inside>::DoSelection();
}

void Select_Touching(void)
{
	SelectByBounds<SelectionPolicy_Touching>::DoSelection(false);
}

void Select_Complete_Tall() {
	SelectByBounds<SelectionPolicy_Complete_Tall>::DoSelection();
}

inline void hide_node(scene::INodePtr node, bool hide) {
  hide
    ? node->enable(scene::Node::eHidden)
    : node->disable(scene::Node::eHidden);
}

class HideSelectedWalker : public scene::Graph::Walker
{
  bool m_hide;
public:
  HideSelectedWalker(bool hide)
    : m_hide(hide)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    Selectable* selectable = Instance_getSelectable(instance);
    if(selectable != 0
      && selectable->isSelected())
    {
      hide_node(path.top(), m_hide);
    }
    return true;
  }
};

void Scene_Hide_Selected(bool hide)
{
  GlobalSceneGraph().traverse(HideSelectedWalker(hide));
}

void Select_Hide()
{
  Scene_Hide_Selected(true);
  SceneChangeNotify();
}

void HideSelected()
{
  Select_Hide();
  GlobalSelectionSystem().setSelectedAll(false);
}


class HideAllWalker : public scene::Graph::Walker
{
  bool m_hide;
public:
  HideAllWalker(bool hide)
    : m_hide(hide)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    hide_node(path.top(), m_hide);
    return true;
  }
};

void Scene_Hide_All(bool hide)
{
  GlobalSceneGraph().traverse(HideAllWalker(hide));
}

void Select_ShowAllHidden()
{
  Scene_Hide_All(false);
  SceneChangeNotify();
}



void Selection_Flipx()
{
  UndoableCommand undo("mirrorSelected -axis x");
  Select_FlipAxis(0);
}

void Selection_Flipy()
{
  UndoableCommand undo("mirrorSelected -axis y");
  Select_FlipAxis(1);
}

void Selection_Flipz()
{
  UndoableCommand undo("mirrorSelected -axis z");
  Select_FlipAxis(2);
}

void Selection_Rotatex()
{
  UndoableCommand undo("rotateSelected -axis x -angle -90");
  Select_RotateAxis(0,-90);
}

void Selection_Rotatey()
{
  UndoableCommand undo("rotateSelected -axis y -angle 90");
  Select_RotateAxis(1, 90);
}

void Selection_Rotatez()
{
  UndoableCommand undo("rotateSelected -axis z -angle -90");
  Select_RotateAxis(2,-90);
}



void Nudge(int nDim, float fNudge)
{
  Vector3 translate(0, 0, 0);
  translate[nDim] = fNudge;
  
  GlobalSelectionSystem().translateSelected(translate);
}

void Selection_NudgeZ(float amount)
{
  StringOutputStream command;
  command << "nudgeSelected -axis z -amount " << amount;
  UndoableCommand undo(command.c_str());

  Nudge(2, amount);
}

void Selection_MoveDown()
{
  Selection_NudgeZ(-GlobalGrid().getGridSize());
}

void Selection_MoveUp()
{
  Selection_NudgeZ(GlobalGrid().getGridSize());
}

void SceneSelectionChange(const Selectable& selectable)
{
  SceneChangeNotify();
}

SignalHandlerId Selection_boundsChanged;

void Selection_construct()
{
  typedef FreeCaller1<const Selectable&, SceneSelectionChange> SceneSelectionChangeCaller;
  GlobalSelectionSystem().addSelectionChangeCallback(SceneSelectionChangeCaller());
  typedef FreeCaller1<const Selectable&, UpdateWorkzone_ForSelectionChanged> UpdateWorkzoneForSelectionChangedCaller;
  GlobalSelectionSystem().addSelectionChangeCallback(UpdateWorkzoneForSelectionChangedCaller());
  typedef FreeCaller<UpdateWorkzone_ForSelection> UpdateWorkzoneForSelectionCaller;
  Selection_boundsChanged = GlobalSceneGraph().addBoundsChangedCallback(UpdateWorkzoneForSelectionCaller());
}

void Selection_destroy()
{
  GlobalSceneGraph().removeBoundsChangedCallback(Selection_boundsChanged);
}
