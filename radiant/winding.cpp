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

#include "winding.h"

#include <algorithm>

#include "brush/FixedWinding.h"
#include "math/line.h"
#include "math/Plane3.h"

/// \brief Returns true if
/// !flipped && winding is completely BACK or ON
/// or flipped && winding is completely FRONT or ON
bool Winding_TestPlane(const Winding& winding, const Plane3& plane, bool flipped) 
{
  const int test = (flipped) ? ePlaneBack : ePlaneFront;
  for(Winding::const_iterator i = winding.begin(); i != winding.end(); ++i)
  {
    if(test == Winding_classifyDistance(plane.distanceToPoint(i->vertex), ON_EPSILON))
    {
      return false;
    }
  }
  return true;
}

/// \brief Returns true if any point in \p w1 is in front of plane2, or any point in \p w2 is in front of plane1
bool Winding_PlanesConcave(const Winding& w1, const Winding& w2, const Plane3& plane1, const Plane3& plane2)
{
  return !Winding_TestPlane(w1, plane2, false) || !Winding_TestPlane(w2, plane1, false);
}

#define DEBUG_EPSILON ON_EPSILON
const double DEBUG_EPSILON_SQUARED = DEBUG_EPSILON * DEBUG_EPSILON;

#define WINDING_DEBUG 0



std::size_t Winding_FindAdjacent(const Winding& winding, std::size_t face)
{
  for(std::size_t i=0; i<winding.numpoints; ++i)
  {
    ASSERT_MESSAGE(winding[i].adjacent != c_brush_maxFaces, "edge connectivity data is invalid");
    if(winding[i].adjacent == face)
    {
      return i;
    }
  }
  return c_brush_maxFaces;
}

std::size_t Winding_Opposite(const Winding& winding, const std::size_t index, const std::size_t other)
{
  ASSERT_MESSAGE(index < winding.numpoints && other < winding.numpoints, "Winding_Opposite: index out of range");

  double dist_best = 0;
  std::size_t index_best = c_brush_maxFaces;

  Ray edge(ray_for_points(winding[index].vertex, winding[other].vertex));

  for(std::size_t i=0; i<winding.numpoints; ++i)
  {
    if(i == index || i == other)
    {
      continue;
    }

    double dist_squared = ray_squared_distance_to_point(edge, winding[i].vertex);

    if(dist_squared > dist_best)
    {
      dist_best = dist_squared;
      index_best = i;
    }
  }
  return index_best;
}

std::size_t Winding_Opposite(const Winding& winding, const std::size_t index)
{
  return Winding_Opposite(winding, index, winding.next(index));
}

/// \brief Calculate the \p centroid of the polygon defined by \p winding which lies on plane \p plane.
void Winding_Centroid(const Winding& winding, const Plane3& plane, Vector3& centroid)
{
  double area2 = 0, x_sum = 0, y_sum = 0;
  const ProjectionAxis axis = projectionaxis_for_normal(plane.normal());
  const indexremap_t remap = indexremap_for_projectionaxis(axis);
  for(std::size_t i = winding.numpoints-1, j = 0; j < winding.numpoints; i = j, ++j)
  {
    const double ai = winding[i].vertex[remap.x] * winding[j].vertex[remap.y] - winding[j].vertex[remap.x] * winding[i].vertex[remap.y];
    area2 += ai;
    x_sum += (winding[j].vertex[remap.x] + winding[i].vertex[remap.x]) * ai;
    y_sum += (winding[j].vertex[remap.y] + winding[i].vertex[remap.y]) * ai;
  }

  centroid[remap.x] = x_sum / (3 * area2);
  centroid[remap.y] = y_sum / (3 * area2);
  {
    Ray ray(Vector3(0, 0, 0), Vector3(0, 0, 0));
    ray.origin[remap.x] = centroid[remap.x];
    ray.origin[remap.y] = centroid[remap.y];
    ray.direction[remap.z] = 1;
    centroid[remap.z] = ray_distance_to_plane(ray, plane);
  }
}
