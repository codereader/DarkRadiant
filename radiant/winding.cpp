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

bool Winding::testPlane(const Plane3& plane, bool flipped) const {
	const int test = (flipped) ? ePlaneBack : ePlaneFront;
	
	for (const_iterator i = begin(); i != end(); ++i) {
		if (test == Winding_classifyDistance(plane.distanceToPoint(i->vertex), ON_EPSILON)) {
			return false;
		}
	}

	return true;
}

bool Winding::planesConcave(const Winding& w1, const Winding& w2, const Plane3& plane1, const Plane3& plane2) {
	return !w1.testPlane(plane2, false) || !w2.testPlane(plane1, false);
}

std::size_t Winding::findAdjacent(std::size_t face) const {
	for (std::size_t i = 0; i < numpoints; ++i) {
		ASSERT_MESSAGE((*this)[i].adjacent != c_brush_maxFaces, "edge connectivity data is invalid");
		if ((*this)[i].adjacent == face) {
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

Vector3 Winding::centroid(const Plane3& plane) const {
	Vector3 centroid(0,0,0);
	
	double area2 = 0, x_sum = 0, y_sum = 0;
	const ProjectionAxis axis = projectionaxis_for_normal(plane.normal());
	const indexremap_t remap = indexremap_for_projectionaxis(axis);
	
	for (std::size_t i = numpoints - 1, j = 0; j < numpoints; i	= j, ++j) {
		const double ai = (*this)[i].vertex[remap.x]
				* (*this)[j].vertex[remap.y] - (*this)[j].vertex[remap.x]
				* (*this)[i].vertex[remap.y];
		
		area2 += ai;
		
		x_sum += ((*this)[j].vertex[remap.x] + (*this)[i].vertex[remap.x]) * ai;
		y_sum += ((*this)[j].vertex[remap.y] + (*this)[i].vertex[remap.y]) * ai;
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
	
	return centroid;
}

void Winding::printConnectivity() {
	for (iterator i = begin(); i != end(); ++i) {
		std::size_t vertexIndex = std::distance(begin(), i);
		globalOutputStream() << "vertex: " << Unsigned(vertexIndex)
			<< " adjacent: " << Unsigned(i->adjacent) << "\n";
	}
}
