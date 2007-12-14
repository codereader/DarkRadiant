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

#if !defined(INCLUDED_WINDING_H)
#define INCLUDED_WINDING_H

#include "debugging/debugging.h"

#include <vector>

#include "iclipper.h"
#include "irender.h"
#include "igl.h"
#include "selectable.h"

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "container/array.h"
#include "texturelib.h"

struct indexremap_t
{
  indexremap_t(std::size_t _x, std::size_t _y, std::size_t _z)
    : x(_x), y(_y), z(_z)
  {
  }
  std::size_t x, y, z;
};

inline indexremap_t indexremap_for_projectionaxis(const ProjectionAxis axis)
{
  switch(axis)
  {
  case eProjectionAxisX: return indexremap_t(1, 2, 0);
  case eProjectionAxisY: return indexremap_t(2, 0, 1);
  default: return indexremap_t(0, 1, 2);
  }
}

const std::size_t c_brush_maxFaces = 1024;

class WindingVertex
{
public:
  Vector3 vertex;
  Vector2 texcoord;
  Vector3 tangent;
  Vector3 bitangent;
  std::size_t adjacent;
};

const double ON_EPSILON	= 1.0 / (1 << 8);

inline PlaneClassification Winding_classifyDistance(const double distance, const double epsilon) {
	if (distance > epsilon) {
		return ePlaneFront;
	}
	
	if (distance < -epsilon) {
		return ePlaneBack;
	}
	
	return ePlaneOn;
}

class Winding
{
public:
  typedef Array<WindingVertex> container_type;

  std::size_t numpoints;
  container_type points;

  typedef container_type::iterator iterator;
  typedef container_type::const_iterator const_iterator;

  Winding() : numpoints(0)
  {
  }
  Winding(std::size_t size) : numpoints(0), points(size)
  {
  }
  void resize(std::size_t size)
  {
    points.resize(size);
    numpoints = 0;
  }

  iterator begin()
  {
    return points.begin();
  }
  const_iterator begin() const
  {
    return points.begin();
  }
  iterator end()
  {
    return points.begin() + numpoints;
  }
  const_iterator end() const
  {
    return points.begin() + numpoints;
  }

  WindingVertex& operator[](std::size_t index)
  {
    ASSERT_MESSAGE(index < points.size(), "winding: index out of bounds");
    return points[index];
  }
  const WindingVertex& operator[](std::size_t index) const
  {
    ASSERT_MESSAGE(index < points.size(), "winding: index out of bounds");
    return points[index];
  }

  void push_back(const WindingVertex& point)
  {
    points[numpoints] = point;
    ++numpoints;
  }
  void erase(iterator point)
  {
    for(iterator i = point + 1; i != end(); point = i, ++i)
    {
      *point = *i;
    }
    --numpoints;
  }
  
	/** greebo: Calculates the AABB of this winding
	 */
	AABB aabb() const {
		AABB returnValue;
		
		for (const_iterator i = begin(); i != end(); i++) {
			returnValue.includePoint(i->vertex);
		}
		
		return returnValue;
	}
	
	void testSelect(SelectionTest& test, SelectionIntersection& best) {
		test.TestPolygon(VertexPointer(reinterpret_cast<VertexPointer::pointer>(&points.data()->vertex), sizeof(WindingVertex)), numpoints, best);
	}
	
	// greebo: Moved the array of normals from the draw() method to here
	// I figured that 256 faces per brush should be enough. This is still FIXME!
	mutable Vector3 normals[256];
	
	// Submits this winding to OpenGL
	inline void draw(const Vector3& normal, RenderStateFlags state) const {
		// Set the vertex pointer first
		glVertexPointer(3, GL_DOUBLE, sizeof(WindingVertex), &points.data()->vertex);

		if ((state & RENDER_BUMP) != 0) {
			typedef Vector3* Vector3Iter;
			
			for (Vector3Iter i = normals, end = normals + numpoints; i != end; ++i) {
				*i = normal;
			}
			
			glVertexAttribPointerARB(11, 3, GL_DOUBLE, 0, sizeof(Vector3), normals);
			glVertexAttribPointerARB(8, 2, GL_DOUBLE, 0, sizeof(WindingVertex), &points.data()->texcoord);
			glVertexAttribPointerARB(9, 3, GL_DOUBLE, 0, sizeof(WindingVertex), &points.data()->tangent);
			glVertexAttribPointerARB(10, 3, GL_DOUBLE, 0, sizeof(WindingVertex), &points.data()->bitangent);
		} 
		else {
			if (state & RENDER_LIGHTING) {
				typedef Vector3* Vector3Iter;
				
				for (Vector3Iter i = normals, last = normals + numpoints; i != last; ++i) {
					*i = normal;
				}
				glNormalPointer(GL_DOUBLE, sizeof(Vector3), normals);
			}

			if (state & RENDER_TEXTURE) {
				glTexCoordPointer(2, GL_DOUBLE, sizeof(WindingVertex), &points.data()->texcoord);
			}
		}
		
		glDrawArrays(GL_POLYGON, 0, GLsizei(numpoints));
	}
	
	// Wraps the given index around if it's larger than the size of this winding
	inline std::size_t wrap(std::size_t i) const {
		ASSERT_MESSAGE(numpoints != 0, "Winding_wrap: empty winding");
		return i % numpoints;
	}
	
	// Returns the next winding index (wraps around)
	inline std::size_t next(std::size_t i) const {
		return wrap(++i);
	}
	
	std::size_t findAdjacent(std::size_t face) const;
	std::size_t opposite(const std::size_t index, const std::size_t other) const;
	std::size_t opposite(std::size_t index) const;
	
	// Returns the classification for the given plane
	BrushSplitType classifyPlane(const Plane3& plane) const {
		BrushSplitType split;
		
		for (const_iterator i = begin(); i != end(); ++i) {
			++split.counts[Winding_classifyDistance(plane.distanceToPoint(i->vertex), ON_EPSILON)];
		}
		
		return split;
	}
	
	/// \brief Returns true if
	/// !flipped && winding is completely BACK or ON
	/// or flipped && winding is completely FRONT or ON
	bool testPlane(const Plane3& plane, bool flipped) const;
	
	// Return the centroid of the polygon defined by this winding lying on the given plane.
	Vector3 centroid(const Plane3& plane) const;
	
	// For debugging purposes: prints the vertices and their adjacents to the console
	void printConnectivity();
	
	/// \brief Returns true if any point in \p w1 is in front of plane2, or any point in \p w2 is in front of plane1
	static bool planesConcave(const Winding& w1, const Winding& w2, const Plane3& plane1, const Plane3& plane2);
};

#endif
