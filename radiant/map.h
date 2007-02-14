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

#if !defined(INCLUDED_MAP_H)
#define INCLUDED_MAP_H

#include "iscenegraph.h"
#include "generic/callback.h"
#include "signal/signalfwd.h"
#include "string/stringfwd.h"

#include <ostream>

class Map;
extern Map g_map;

class MapFormat;

void Map_addValidCallback(Map& map, const SignalHandler& handler);
bool Map_Valid(const Map& map);

class DeferredDraw
{
  Callback m_draw;
  bool m_defer;
  bool m_deferred;
public:
  DeferredDraw(const Callback& draw) : m_draw(draw), m_defer(false), m_deferred(false)
  {
  }
  void defer()
  {
    m_defer = true;
  }
  void draw()
  {
    if(m_defer)
    {
      m_deferred = true;
    }
    else
    {
      m_draw();
    }
  }
  void flush()
  {
    if(m_defer && m_deferred)
    {
      m_draw();
    }
    m_deferred = false;
    m_defer = false;
  }
};

inline void DeferredDraw_onMapValidChanged(DeferredDraw& self)
{
  if(Map_Valid(g_map))
  {
    self.flush();
  }
  else
  {
    self.defer();
  }
}
typedef ReferenceCaller<DeferredDraw, DeferredDraw_onMapValidChanged> DeferredDrawOnMapValidChangedCaller;



const MapFormat& Map_getFormat(const Map& map);
bool Map_Unnamed(const Map& map);


namespace scene
{
  class Node;
  class Graph;
}

scene::Node* Map_GetWorldspawn(const Map& map);
scene::Node* Map_FindWorldspawn(Map& map);
scene::Node& Map_FindOrInsertWorldspawn(Map& map);

template<typename Element> class BasicVector3;
typedef BasicVector3<float> Vector3;

extern Vector3 region_mins, region_maxs;
extern bool region_active;

void Map_LoadFile(const std::string& filename);
bool Map_SaveFile(const char* filename);

void Map_New();
void Map_Free();

void Map_RegionOff();

bool Map_SaveRegion(const char* filename);

class TextInputStream;
class TextOutputStream;

// Map import and export functions
void Map_ImportSelected(TextInputStream& in, const MapFormat& format);
void Map_ExportSelected(std::ostream& out, const MapFormat& format);

bool Map_Modified(const Map& map);

bool Map_Save();
bool Map_SaveAs();

scene::Node& Node_Clone(scene::Node& node);

void DoMapInfo();

void Scene_parentSelectedBrushesToEntity(scene::Graph& graph, scene::Node& parent);

void Scene_parentSelected();

void OnUndoSizeChanged();

void NewMap();
void OpenMap();
void ImportMap();
void SaveMapAs();
void SaveMap();
void ExportMap();
void SaveRegion();


void Map_Traverse(scene::Node& root, const scene::Traversable::Walker& walker);


void SelectBrush (int entitynum, int brushnum);

void Map_Construct();
void Map_Destroy();


void Map_gatherNamespaced(scene::Node& root);
void Map_mergeClonedNames();


const char* getMapsPath();

namespace map {

	/**
	 * Return the filename of the current map, as a string.
	 */
	std::string getFileName();

	/** Subtract the provided origin vector from all selected brushes. This is 
	 * necessary when reparenting worldspawn brushes to an entity, since the entity's
	 * "origin" key will be added to all child brushes.
	 * 
	 * @param origin
	 * Vector3 containing the new origin for the selected brushes.
	 */
	 
	void selectedBrushesSubtractOrigin(const Vector3& origin);
	
	/** Count the number of selected primitives in the current map.
	 * 
	 * @returns
	 * The number of selected primitives.
	 */
	 
	int countSelectedPrimitives();
	
	/** Count the number of selected brushes in the current map.
	 * 
	 * @returns
	 * The number of selected brushes.
	 */
	 
	int countSelectedBrushes();
	
	/**
	 * Set the "modified" flag on the current map, to allow saving.
	 */
	void setModified();
	
	/** greebo: Focus the XYViews and the Camera to the given point/angle.
	 */
	void focusViews(const Vector3& point, const Vector3& angles);
}


#endif
