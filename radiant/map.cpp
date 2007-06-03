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

#include "map/Map.h"

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

class SelectionExcluder : public Excluder
{
public:
  bool excluded(scene::INodePtr node) const
  {
    return !Node_selectedDescendant(node);
  }
};

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
	map::Map::registerCommands();
	
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
