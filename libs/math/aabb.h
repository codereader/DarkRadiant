#pragma once

#include "math/Matrix4.h"
#include "math/Plane3.h"

/** 
 * An Axis Aligned Bounding Box is a simple cuboid which encloses a given set
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
	AABB() : 
		origin(0, 0, 0), 
		extents(-1,-1,-1)
	{}

	/** Construct an AABB with the provided origin and extents
	 * vectors.
	 */
	AABB(const Vector3& origin_, const Vector3& extents_) : 
		origin(origin_), 
		extents(extents_)
	{}

	/** 
	 * Static named constructor to create an AABB that encloses the provided
	 * minimum and maximum points.
	 */
	static AABB createFromMinMax(const Vector3& min, const Vector3& max);

	/**
	 * Static named constructor to create an AABB that encloses the given AABB rotated
	 * by the given transformation matrix.
	 */
	static AABB createFromOrientedAABB(const AABB& aabb, const Matrix4& transform);

	/**
	 * Static named constructor to create an AABB that encloses the given AABB rotated
	 * by the given transformation matrix. If the given AABB is not valid, it is returned as it is.
	 */
	static AABB createFromOrientedAABBSafe(const AABB& aabb, const Matrix4& transform);

	/**
	 * Create an AABB that is as large as possible (extents are filled with FLT_MAX)
	 */
	static AABB createInfinite();

	/**
	 * Equality operator, returns true if both origina nd extents are exactly the same
	 */
	bool operator==(const AABB& other) const;

	/**
	 * Inequality operator, using this is equivalent to calling !(operator==)
	 */
	bool operator!=(const AABB& other) const;

	/**
	 * Check whether the AABB is valid, or if the extents are still uninitialised
	 */
	bool isValid() const;

	/** Get the origin of this AABB.
	 *
	 * @returns
	 * A const reference to a Vector3 containing the AABB's origin.
	 */
	const Vector3& getOrigin() const;

	/** Get the extents of this AABB.
	 *
	 * @returns
	 * A const reference to a Vector3 containing the AABB's extents.
	 */
	const Vector3& getExtents() const;

	/** Get the radius of the smallest sphere which encloses this
	 * bounding box.
	 */
	float getRadius() const;

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

	/**
	 * Extends this AABB by the given vector's length.
	 * Equivalent to aabb.extents += extension
	 */
	void extendBy(const Vector3& extension);

	/**
	 * Returns true if this AABB contains the other AABB (all dimensions)
	 */
	bool contains(const AABB& other) const;

	/**
	 * Classifies the position of this AABB with respect to the given plane.
	 *
	 * TODO: Better documentation. TODO: Use enum as return value
	 * 
	 * @returns: 0 = totally outside, 1 = partially inside, 2 = totally inside
	 */
	unsigned int classifyPlane(const Plane3& plane) const;

	/**
	 * Like classifyPlane, but uses the given matrix to transform the plane.
	 *
	 * TODO: Better documentation. TODO: Use enum as return value
	 * 
	 * @returns: 0 = totally outside, 1 = partially inside, 2 = totally inside
	 */
	unsigned int classifyOrientedPlane(const Matrix4& transform, const Plane3& plane) const;

	/**
	 * Stores the 3D coordinates of the AABB corner points into the given array.
	 */
	void getCorners(Vector3 corners[8]) const;

	/**
	 * Stores the 3D coordinates of the rotated AABB corner points into the given array.
	 * The rotation is performed using the given transformation matrix.
	 */
	void getCorners(Vector3 corners[8], const Matrix4& rotation) const;

	/**
	 * Writes the 6 Plane3 objects into the given array, corresponding to the
	 * six sides of this AABB.
	 */
	void getPlanes(Plane3 planes[6]) const;

	/**
	 * Writes the 6 Plane3 objects into the given array, corresponding to the
	 * six rotated sides of this AABB. The given matrix is used to rotate this AABB.
	 */
	void getPlanes(Plane3 planes[6], const Matrix4& rotation) const;
};

inline AABB AABB::createFromMinMax(const Vector3& min, const Vector3& max)
{
	// Origin is the midpoint of the two vectors
	Vector3 origin = (min + max) * 0.5f;

	// Extents is the vector from the origin to the max point
	Vector3 extents = max - origin;

	// Construct and return the resulting AABB;
	return AABB(origin, extents);
}

inline AABB AABB::createFromOrientedAABBSafe(const AABB& aabb, const Matrix4& transform)
{
	return aabb.isValid() ? createFromOrientedAABB(aabb, transform) : aabb;
}

inline AABB AABB::createInfinite()
{
	return AABB(Vector3(0, 0, 0), Vector3(FLT_MAX, FLT_MAX, FLT_MAX));
}

inline bool AABB::operator==(const AABB& other) const
{
	return (origin == other.origin && extents == other.extents);
}

inline bool AABB::operator!=(const AABB& other) const
{
	return !operator==(other);
}

inline bool AABB::isValid() const
{
	// Check each origin and extents value. The origins must be between
	// +/- FLT_MAX, and the extents between 0 and FLT_MAX.
	for (int i = 0; i < 3; ++i)
	{
		if (origin[i] < -FLT_MAX || origin[i] > FLT_MAX || 
			extents[i] < 0 || extents[i] > FLT_MAX)
		{
			return false;
		}
	}

	return true; // all checks passed
}

inline const Vector3& AABB::getOrigin() const
{
	return origin;
}

inline const Vector3& AABB::getExtents() const
{
	return extents;
}

inline float AABB::getRadius() const
{
	return extents.getLength(); // Pythagorean length of extents vector
}

inline bool AABB::contains(const AABB& other) const
{
	// Return true if all coordinates of <other> are contained within these bounds
	return (origin[0] + extents[0] >= other.origin[0] + other.extents[0]) &&
			(origin[0] - extents[0] <= other.origin[0] - other.extents[0]) &&
			(origin[1] + extents[1] >= other.origin[1] + other.extents[1]) &&
			(origin[1] - extents[1] <= other.origin[1] - other.extents[1]) &&
			(origin[2] + extents[2] >= other.origin[2] + other.extents[2]) &&
			(origin[2] - extents[2] <= other.origin[2] - other.extents[2]);
}

inline void AABB::extendBy(const Vector3& extension)
{
	extents += extension;
}

inline unsigned int AABB::classifyPlane(const Plane3& plane) const
{
	float distance_origin = plane.normal().dot(origin) + plane.dist();

	if (fabs(distance_origin) < (fabs(plane.normal().x() * extents[0]) + 
								 fabs(plane.normal().y() * extents[1]) + 
								 fabs(plane.normal().z() * extents[2])))
	{
		return 1; // partially inside
	}
	else if (distance_origin < 0)
	{
		return 2; // totally inside
	}

	return 0; // totally outside
}

inline void AABB::getCorners(Vector3 corners[8]) const
{
	Vector3 min(origin - extents);
	Vector3 max(origin + extents);

	corners[0] = Vector3(min[0], max[1], max[2]);
	corners[1] = Vector3(max[0], max[1], max[2]);
	corners[2] = Vector3(max[0], min[1], max[2]);
	corners[3] = Vector3(min[0], min[1], max[2]);
	corners[4] = Vector3(min[0], max[1], min[2]);
	corners[5] = Vector3(max[0], max[1], min[2]);
	corners[6] = Vector3(max[0], min[1], min[2]);
	corners[7] = Vector3(min[0], min[1], min[2]);
}

inline void AABB::getPlanes(Plane3 planes[6]) const
{
	planes[0] = Plane3( g_vector3_axes[0], origin[0] + extents[0]);
	planes[1] = Plane3(-g_vector3_axes[0], -(origin[0] - extents[0]));
	planes[2] = Plane3( g_vector3_axes[1], origin[1] + extents[1]);
	planes[3] = Plane3(-g_vector3_axes[1], -(origin[1] - extents[1]));
	planes[4] = Plane3( g_vector3_axes[2], origin[2] + extents[2]);
	planes[5] = Plane3(-g_vector3_axes[2], -(origin[2] - extents[2]));
}

/**
 * Stream insertion for AABB class.
 */
inline std::ostream& operator<< (std::ostream& os, const AABB& aabb)
{
	os << "AABB { origin=" << aabb.getOrigin() << ", extents=" << aabb.getExtents() << " }";

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

/// \brief A compile-time-constant integer.
template<int VALUE_>
struct IntegralConstant
{
  enum unnamed_{ VALUE = VALUE_ };
};


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

#if 0
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
  float x = rotation.x().getVector3().dot(aabb.origin);
  float y = rotation.y().getVector3().dot(aabb.origin);
  float z = rotation.z().getVector3().dot(aabb.origin);

  planes[0] = Plane3(rotation.x().getVector3(), x + aabb.extents[0]);
  planes[1] = Plane3(-rotation.x().getVector3(), -(x - aabb.extents[0]));
  planes[2] = Plane3(rotation.y().getVector3(), y + aabb.extents[1]);
  planes[3] = Plane3(-rotation.y().getVector3(), -(y - aabb.extents[1]));
  planes[4] = Plane3(rotation.z().getVector3(), z + aabb.extents[2]);
  planes[5] = Plane3(-rotation.z().getVector3(), -(z - aabb.extents[2]));
}
#endif

const Vector3 aabb_normals[6] = {
  Vector3( 1, 0, 0 ),
  Vector3( 0, 1, 0 ),
  Vector3( 0, 0, 1 ),
  Vector3(-1, 0, 0 ),
  Vector3( 0,-1, 0 ),
  Vector3( 0, 0,-1 ),
};

const float aabb_texcoord_topleft[2] = { 0, 0 };
const float aabb_texcoord_topright[2] = { 1, 0 };
const float aabb_texcoord_botleft[2] = { 0, 1 };
const float aabb_texcoord_botright[2] = { 1, 1 };

#if 0
inline AABB aabb_for_oriented_aabb(const AABB& aabb, const Matrix4& transform)
{
  return AABB(
    transform.transformPoint(aabb.origin),
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
  return AABB(Vector3(0, 0, 0), Vector3(FLT_MAX, FLT_MAX, FLT_MAX));
}
#endif
