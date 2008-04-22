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

#if !defined (INCLUDED_BRUSHWRAPPER_H)
#define INCLUDED_BRUSHWRAPPER_H

#include <cstddef>
#include <string>
#include "inode.h"
#include "generic/callbackfwd.h"
#include "math/Vector3.h"

enum EBrushPrefab
{
  eBrushCuboid,
  eBrushPrism,
  eBrushCone,
  eBrushSphere,
};

class Brush;
namespace scene
{
  class Graph;
}
void Scene_BrushConstructPrefab(scene::Graph& graph, EBrushPrefab type, std::size_t sides, const std::string& shader);
class AABB;
void Scene_BrushResize(Brush& brush, const AABB& bounds, const std::string& shader);
void Scene_BrushResize_Selected(scene::Graph& graph, const AABB& bounds, const std::string& shader);
void Scene_BrushSetShader_Selected(scene::Graph& graph, const std::string& name);
void Scene_BrushSetShader_Component_Selected(scene::Graph& graph, const std::string& name);
void Scene_BrushSelectByShader(scene::Graph& graph, const std::string& name);
void Scene_BrushSelectByShader_Component(scene::Graph& graph, const std::string& name);
void Scene_BrushFlipTexture_Selected(unsigned int flipAxis);
void Scene_BrushFlipTexture_Component_Selected(unsigned int flipAxis);

/** greebo: Constructs the region boundary brushes
 */
void ConstructRegionBrushes(scene::INodePtr brushes[6], const Vector3& region_mins, const Vector3& region_maxs);

void Brush_registerCommands();

#endif
