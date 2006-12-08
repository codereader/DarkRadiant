/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

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

#if !defined(INCLUDED_MATH_LINE_H)
#define INCLUDED_MATH_LINE_H

/// \file
/// \brief Line data types and related operations.

#include "math/Plane3.h"
#include "math/matrix.h"

/// \brief A line segment defined by a start point and and end point.
class Line
{
public:
  Vector3 start, end;

  Line()
  {
  }
  Line(const Vector3& start_, const Vector3& end_) : start(start_), end(end_)
  {
  }
};

inline Vector3 line_closest_point(const Line& line, const Vector3& point)
{
  Vector3 v = line.end - line.start;
  Vector3 w = point - line.start;

  double c1 = w.dot(v);
  if ( c1 <= 0 )
    return line.start;

  double c2 = v.dot(v);
  if ( c2 <= c1 )
    return line.end;

  return Vector3(line.start + v * (c1 / c2));
}


class Segment
{
public:
  Vector3 origin, extents;

  Segment()
  {
  }
  Segment(const Vector3& origin_, const Vector3& extents_) :
    origin(origin_), extents(extents_)
  {
  }
};


inline Segment segment_for_startend(const Vector3& start, const Vector3& end)
{
  Segment segment;
  segment.origin = vector3_mid(start, end);
  segment.extents = end - segment.origin;
  return segment;
}

inline unsigned int segment_classify_plane(const Segment& segment, const Plane3& plane)
{
  double distance_origin = plane.normal().dot(segment.origin) + plane.dist();
  
  if (fabs(distance_origin) < fabs(plane.normal().dot(segment.extents)))
  {
    return 1; // partially inside
  }
  else if (distance_origin < 0)
  {
    return 2; // totally inside
  }
  return 0; // totally outside
}


class Ray
{
public:
  Vector3 origin, direction;

  Ray()
  {
  }
  Ray(const Vector3& origin_, const Vector3& direction_) :
    origin(origin_), direction(direction_)
  {
  }
  
	/* greebo: this calculates the intersection point of two rays 
	 * (copied from Radiant's Intersection code, there may be better ways) 
	 */
	Vector3 getIntersection(Ray& other) {
		Vector3 intersection = origin - other.origin;
		
		double dot = direction.dot(other.direction);
  		double d = direction.dot(intersection);
		double e = other.direction.dot(intersection);
		double D = 1 - dot*dot;       // always >= 0

		if (D < 0.000001) {
			// the lines are almost parallel
			return other.origin + other.direction*e;
		}
		else {
			return other.origin + other.direction*((e - dot*d) / D);
		}
	}
};

inline Ray ray_for_points(const Vector3& origin, const Vector3& p2)
{
  return Ray(origin, (p2 - origin).getNormalised());
}

inline void ray_transform(Ray& ray, const Matrix4& matrix)
{
  matrix4_transform_point(matrix, ray.origin);
  matrix4_transform_direction(matrix, ray.direction);
}

// closest-point-on-line
inline double ray_squared_distance_to_point(const Ray& ray, const Vector3& point)
{
  return (point - (ray.origin + ray.direction * (point - ray.origin).dot(ray.direction))).getLengthSquared();
}

inline double ray_distance_to_plane(const Ray& ray, const Plane3& plane)
{
  return -(plane.normal().dot(ray.origin) - plane.dist()) / ray.direction.dot(plane.normal());
}

#endif
