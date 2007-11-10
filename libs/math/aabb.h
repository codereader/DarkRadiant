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

#if !defined(INCLUDED_MATH_AABB_H)
#define INCLUDED_MATH_AABB_H

#include "math/matrix.h"
#include "math/Plane3.h"

/** An Axis Aligned Bounding Box is a simple cuboid which encloses a given set 
 * of points, such as the vertices of a model. It is defined by an origin, 
 * located at the centre of the AABB, and symmetrical extents in 3 dimension 
 * which determine its size.
 */
class AABB
{
public:
  	/// The origin of the AABB, which is always located at the centre.
  	Vector3 origin;
  	
  	/// The symmetrical extents in 3 dimensions.
  	Vector3 extents;

	/** Construct an AABB with default origin and invalid extents.
	 */
	AABB() 
	: origin(0, 0, 0), extents(-1,-1,-1) {}

	/** Construct an AABB with the provided origin and extents
	 * vectors.
	 */
	AABB(const Vector3& origin_, const Vector3& extents_) 
	: origin(origin_), extents(extents_) {}
  
	/** Static named constructor to create an AABB that encloses the provided
	 * minimum and maximum points.
	 */
	static AABB createFromMinMax(const Vector3& min, const Vector3& max);

	/** Determine whether this AABB is valid.
	 */
	bool isValid() const;

	/** Get the origin of this AABB.
	 * 
	 * @returns
	 * A const reference to a Vector3 containing the AABB's origin.
	 */
	const Vector3& getOrigin() const {
		return origin;
	}
	
	/** Get the extents of this AABB.
	 * 
	 * @returns
	 * A const reference to a Vector3 containing the AABB's extents.
	 */
	const Vector3& getExtents() const {
		return extents;
	}

	/** Get the radius of the smallest sphere which encloses this
	 * bounding box.
	 */
	double getRadius() const {
		return extents.getLength(); // Pythagorean length of extents vector	
	}

	/** Expand this AABB in-place to include the given point in
	 * world space.
	 * 
	 * @param point
	 * Vector3 representing the point to include.
	 */
	void includePoint(const Vector3& point);	 

	/** Expand this AABB in-place to include the given AABB in
	 * world space.
	 * 
	 * @param other
	 * The other AABB to include.
	 */
	void includeAABB(const AABB& other);	 
};

/**
 * Stream insertion for AABB class.
 */
inline 
std::ostream& operator<< (std::ostream& os, const AABB& aabb) {
	os << "AABB { origin=" << aabb.getOrigin() 
	   << ", extents=" << aabb.getExtents() << " }";
	return os;
}

class AABBExtendByPoint
{
  AABB& m_aabb;
public:
  AABBExtendByPoint(AABB& aabb) : m_aabb(aabb)
  {
  }
  void operator()(const Vector3& point) const
  {
    m_aabb.includePoint(point);
  }
};
  
inline void aabb_extend_by_vec3(AABB& aabb, const Vector3& extension)
{
  aabb.extents += extension;
}




template<typename Index>
inline bool aabb_intersects_point_dimension(const AABB& aabb, const Vector3& point)
{
  return fabs(point[Index::VALUE] - aabb.origin[Index::VALUE]) < aabb.extents[Index::VALUE];
}

inline bool aabb_intersects_point(const AABB& aabb, const Vector3& point)
{
  return aabb_intersects_point_dimension< IntegralConstant<0> >(aabb, point)
    && aabb_intersects_point_dimension< IntegralConstant<1> >(aabb, point)
    && aabb_intersects_point_dimension< IntegralConstant<2> >(aabb, point);
}

template<typename Index>
inline bool aabb_intersects_aabb_dimension(const AABB& aabb, const AABB& other)
{
  return fabs(other.origin[Index::VALUE] - aabb.origin[Index::VALUE]) < (aabb.extents[Index::VALUE] + other.extents[Index::VALUE]);
}

inline bool aabb_intersects_aabb(const AABB& aabb, const AABB& other)
{
  return aabb_intersects_aabb_dimension< IntegralConstant<0> >(aabb, other)
    && aabb_intersects_aabb_dimension< IntegralConstant<1> >(aabb, other)
    && aabb_intersects_aabb_dimension< IntegralConstant<2> >(aabb, other);
}

inline unsigned int aabb_classify_plane(const AABB& aabb, const Plane3& plane)
{
  double distance_origin = plane.normal().dot(aabb.origin) + plane.dist();
  
  if(fabs(distance_origin) < (fabs(plane.a * aabb.extents[0])
    + fabs(plane.b * aabb.extents[1])
    + fabs(plane.c * aabb.extents[2])))
  {
    return 1; // partially inside
  }
  else if (distance_origin < 0)
  {
    return 2; // totally inside
  }
  return 0; // totally outside
}

inline unsigned int aabb_oriented_classify_plane(const AABB& aabb, const Matrix4& transform, const Plane3& plane)
{
  double distance_origin = plane.normal().dot(aabb.origin) + plane.dist();

  if(fabs(distance_origin) < (fabs(aabb.extents[0] * plane.normal().dot(transform.x().getVector3()) )
    + fabs(aabb.extents[1] * plane.normal().dot(transform.y().getVector3()))
    + fabs(aabb.extents[2] * plane.normal().dot(transform.z().getVector3()))))
  {
    return 1; // partially inside
  }
  else if (distance_origin < 0)
  {
    return 2; // totally inside
  }
  return 0; // totally outside
}

inline void aabb_corners(const AABB& aabb, Vector3 corners[8])
{
  Vector3 min(aabb.origin - aabb.extents);
  Vector3 max(aabb.origin + aabb.extents);
  corners[0] = Vector3(min[0], max[1], max[2]);
  corners[1] = Vector3(max[0], max[1], max[2]);
  corners[2] = Vector3(max[0], min[1], max[2]);
  corners[3] = Vector3(min[0], min[1], max[2]);
  corners[4] = Vector3(min[0], max[1], min[2]);
  corners[5] = Vector3(max[0], max[1], min[2]);
  corners[6] = Vector3(max[0], min[1], min[2]);
  corners[7] = Vector3(min[0], min[1], min[2]);
}

inline void aabb_corners_oriented(const AABB& aabb, const Matrix4& rotation, Vector3 corners[8])
{
  Vector3 x = rotation.x().getVector3() * aabb.extents.x();
  Vector3 y = rotation.y().getVector3() * aabb.extents.y();
  Vector3 z = rotation.z().getVector3() * aabb.extents.z();

  corners[0] = aabb.origin + -x +  y +  z;
  corners[1] = aabb.origin +  x +  y +  z;
  corners[2] = aabb.origin +  x + -y +  z;
  corners[3] = aabb.origin + -x + -y +  z;
  corners[4] = aabb.origin + -x +  y + -z;
  corners[5] = aabb.origin +  x +  y + -z;
  corners[6] = aabb.origin +  x + -y + -z;
  corners[7] = aabb.origin + -x + -y + -z;
}

inline void aabb_planes(const AABB& aabb, Plane3 planes[6])
{
  planes[0] = Plane3(g_vector3_axes[0], aabb.origin[0] + aabb.extents[0]);
  planes[1] = Plane3(-g_vector3_axes[0], -(aabb.origin[0] - aabb.extents[0]));
  planes[2] = Plane3(g_vector3_axes[1], aabb.origin[1] + aabb.extents[1]);
  planes[3] = Plane3(-g_vector3_axes[1], -(aabb.origin[1] - aabb.extents[1]));
  planes[4] = Plane3(g_vector3_axes[2], aabb.origin[2] + aabb.extents[2]);
  planes[5] = Plane3(-g_vector3_axes[2], -(aabb.origin[2] - aabb.extents[2]));
}

inline void aabb_planes_oriented(const AABB& aabb, const Matrix4& rotation, Plane3 planes[6])
{
  double x = rotation.x().getVector3().dot(aabb.origin);
  double y = rotation.y().getVector3().dot(aabb.origin);
  double z = rotation.z().getVector3().dot(aabb.origin);

  planes[0] = Plane3(rotation.x().getVector3(), x + aabb.extents[0]);
  planes[1] = Plane3(-rotation.x().getVector3(), -(x - aabb.extents[0]));
  planes[2] = Plane3(rotation.y().getVector3(), y + aabb.extents[1]);
  planes[3] = Plane3(-rotation.y().getVector3(), -(y - aabb.extents[1]));
  planes[4] = Plane3(rotation.z().getVector3(), z + aabb.extents[2]);
  planes[5] = Plane3(-rotation.z().getVector3(), -(z - aabb.extents[2]));
}

const Vector3 aabb_normals[6] = {
  Vector3( 1, 0, 0 ),
  Vector3( 0, 1, 0 ),
  Vector3( 0, 0, 1 ),
  Vector3(-1, 0, 0 ),
  Vector3( 0,-1, 0 ),
  Vector3( 0, 0,-1 ),
};

const double aabb_texcoord_topleft[2] = { 0, 0 };
const double aabb_texcoord_topright[2] = { 1, 0 };
const double aabb_texcoord_botleft[2] = { 0, 1 };
const double aabb_texcoord_botright[2] = { 1, 1 };


inline AABB aabb_for_oriented_aabb(const AABB& aabb, const Matrix4& transform)
{
  return AABB(
    matrix4_transformed_point(transform, aabb.origin),
    Vector3(
      fabs(transform[0]  * aabb.extents[0]) + 
      	fabs(transform[4]  * aabb.extents[1]) + 
      	fabs(transform[8]  * aabb.extents[2]),
      fabs(transform[1]  * aabb.extents[0]) + 
      	fabs(transform[5]  * aabb.extents[1]) + 
      	fabs(transform[9]  * aabb.extents[2]),
      fabs(transform[2]  * aabb.extents[0]) + 
      	fabs(transform[6]  * aabb.extents[1]) + 
      	fabs(transform[10] * aabb.extents[2])
    )
  );
}

inline AABB aabb_for_oriented_aabb_safe(const AABB& aabb, const Matrix4& transform)
{
  if(aabb.isValid())
  {
    return aabb_for_oriented_aabb(aabb, transform);
  }
  return aabb;
}

inline AABB aabb_infinite()
{
  return AABB(Vector3(0, 0, 0), Vector3(DBL_MAX, DBL_MAX, DBL_MAX));
}

#endif
