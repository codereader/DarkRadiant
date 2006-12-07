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

#if !defined(INCLUDED_MATH_PLANE_H)
#define INCLUDED_MATH_PLANE_H

/// \file
/// \brief Plane data types and related operations.

#include "math/matrix.h"

/// \brief A plane equation stored in double-precision floating-point.
class Plane3
{
public:
  double a, b, c, d;

  Plane3()
  {
  }
  Plane3(double _a, double _b, double _c, double _d)
    : a(_a), b(_b), c(_c), d(_d)
  {
  }
  template<typename Element>
  Plane3(const BasicVector3<Element>& normal, double dist)
    : a(normal.x()), b(normal.y()), c(normal.z()), d(dist)
  {
  }

  BasicVector3<double>& normal()
  {
    return reinterpret_cast<BasicVector3<double>&>(*this);
  }
  const BasicVector3<double>& normal() const
  {
    return reinterpret_cast<const BasicVector3<double>&>(*this);
  }
  double& dist()
  {
    return d;
  }
  const double& dist() const
  {
    return d;
  }
};

/* greebo: This calculates the intersection point of three planes. 
 * Returns <0,0,0> if no intersection point could be found, otherwise returns the coordinates of the intersection point (this may also be 0,0,0)
 */
inline Vector3 intersectPlanes(const Plane3& plane1, const Plane3& plane2, const Plane3& plane3) {
	const Vector3& n1 = plane1.normal();
	const Vector3& n2 = plane2.normal();
	const Vector3& n3 = plane3.normal();
	
	Vector3 n1n2 = n1.crossProduct(n2);
	Vector3 n2n3 = n2.crossProduct(n3);
	Vector3 n3n1 = n3.crossProduct(n1);
	
	double denom = n1.dot(n2n3);
	
	// Check if the denominator is zero (which would mean that no intersection is to be found
	if (denom != 0) {
		return (n2n3*plane1.dist() + n3n1*plane2.dist() + n1n2*plane3.dist()) / denom;
	}
	else {
		// No intersection could be found, return <0,0,0>
		return Vector3(0,0,0);
	}
}

inline Plane3 plane3_normalised(const Plane3& plane)
{
  double rmagnitude = 1.0 / sqrt(plane.a * plane.a + plane.b * plane.b + plane.c * plane.c);
  return Plane3(
    plane.a * rmagnitude,
    plane.b * rmagnitude,
    plane.c * rmagnitude,
    plane.d * rmagnitude
  );
}

inline Plane3 plane3_translated(const Plane3& plane, const Vector3& translation)
{
  Plane3 transformed;
  transformed.a = plane.a;
  transformed.b = plane.b;
  transformed.c = plane.c;
  transformed.d = -((-plane.d * transformed.a + translation.x()) * transformed.a + 
              (-plane.d * transformed.b + translation.y()) * transformed.b + 
              (-plane.d * transformed.c + translation.z()) * transformed.c);
  return transformed;
}

inline Plane3 plane3_transformed(const Plane3& plane, const Matrix4& transform)
{
  Plane3 transformed;
  transformed.a = transform[0] * plane.a + transform[4] * plane.b + transform[8] * plane.c;
  transformed.b = transform[1] * plane.a + transform[5] * plane.b + transform[9] * plane.c;
  transformed.c = transform[2] * plane.a + transform[6] * plane.b + transform[10] * plane.c;
  transformed.d = -((-plane.d * transformed.a + transform[12]) * transformed.a + 
              (-plane.d * transformed.b + transform[13]) * transformed.b + 
              (-plane.d * transformed.c + transform[14]) * transformed.c);
  return transformed;
}

inline Plane3 plane3_inverse_transformed(const Plane3& plane, const Matrix4& transform)
{
  return Plane3
  (
    transform[ 0] * plane.a + transform[ 1] * plane.b + transform[ 2] * plane.c + transform[ 3] * plane.d,
    transform[ 4] * plane.a + transform[ 5] * plane.b + transform[ 6] * plane.c + transform[ 7] * plane.d,
    transform[ 8] * plane.a + transform[ 9] * plane.b + transform[10] * plane.c + transform[11] * plane.d,
    transform[12] * plane.a + transform[13] * plane.b + transform[14] * plane.c + transform[15] * plane.d
  );
}

inline Plane3 plane3_flipped(const Plane3& plane)
{
  return Plane3(-plane.normal(), -plane.dist());
}

const double c_PLANE_NORMAL_EPSILON = 0.0001f;
const double c_PLANE_DIST_EPSILON = 0.02;

inline bool plane3_equal(const Plane3& self, const Plane3& other)
{
  return vector3_equal_epsilon(self.normal(), other.normal(), c_PLANE_NORMAL_EPSILON)
	  && float_equal_epsilon(self.dist(), other.dist(), c_PLANE_DIST_EPSILON);
}

inline bool plane3_opposing(const Plane3& self, const Plane3& other)
{
  return plane3_equal(self, plane3_flipped(other));
}

inline bool plane3_valid(const Plane3& self)
{
  return float_equal_epsilon(self.normal().dot(self.normal()), 1.0, 0.01);
}

template<typename Element>
inline Plane3 plane3_for_points(const BasicVector3<Element>& p0, const BasicVector3<Element>& p1, const BasicVector3<Element>& p2)
{
	Plane3 self;
  	self.normal() = (p1 - p0).crossProduct(p2 - p0).getNormalised();
	self.dist() = p0.dot(self.normal());
	return self;
}

template<typename Element>
inline Plane3 plane3_for_points(const BasicVector3<Element> planepts[3])
{
	return plane3_for_points(planepts[2], planepts[1], planepts[0]);
}


#endif
