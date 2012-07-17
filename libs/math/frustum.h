#pragma once

/// \file
/// \brief View-frustum data types and related operations.

#include "math/Matrix4.h"
#include "math/Segment.h"

#include "VolumeIntersectionValue.h"

class AABB;
class Plane3;

/**
 * \brief
 * Object representing a frustum, defined by six planes.
 */
class Frustum
{
public:
	Plane3 right, left, bottom, top, back, front;

	Frustum()
	{}

	Frustum(const Plane3& _right, const Plane3& _left, 
			const Plane3& _bottom, const Plane3& _top,
			const Plane3& _back, const Plane3& _front) : 
		right(_right), 
		left(_left), 
		bottom(_bottom), 
		top(_top), 
		back(_back), 
		front(_front)
	{}

	/**
	 * Construct the frustum planes from the given projection matrix.
	 */
	static Frustum createFromViewproj(const Matrix4& viewproj);

    /**
     * \brief
     * Normalise all planes in the frustum.
     */
    void normalisePlanes();

    /**
     * \brief
     * Get the projection matrix corresponding to the planes of this frustum.
     */
    Matrix4 getProjectionMatrix() const;

    /**
     * \brief
     * Return a copy of this frustum transformed by the given matrix.
     */
    Frustum getTransformedBy(const Matrix4& transform) const;

    /**
     * \brief
     * Test the intersection of this frustum with an AABB.
     */
    VolumeIntersectionValue testIntersection(const AABB& aabb) const;

	/**
	 * Test the intersection of this frustum with a transformed AABB.
	 */
	VolumeIntersectionValue testIntersection(const AABB& aabb, const Matrix4& localToWorld) const;

	/**
	 * Returns true if the given point is contained in this frustum.
	 */
	bool testPoint(const Vector3& point) const;

	bool testLine(const Segment& segment) const;
};

inline Frustum Frustum::createFromViewproj(const Matrix4& viewproj)
{
	return Frustum
	(
		Plane3(viewproj[3] - viewproj[0], viewproj[7] - viewproj[4], viewproj[11] - viewproj[ 8], viewproj[15] - viewproj[12]).getNormalised(),
		Plane3(viewproj[3] + viewproj[0], viewproj[7] + viewproj[4], viewproj[11] + viewproj[ 8], viewproj[15] + viewproj[12]).getNormalised(),
		Plane3(viewproj[3] + viewproj[1], viewproj[7] + viewproj[5], viewproj[11] + viewproj[ 9], viewproj[15] + viewproj[13]).getNormalised(),
		Plane3(viewproj[3] - viewproj[1], viewproj[7] - viewproj[5], viewproj[11] - viewproj[ 9], viewproj[15] - viewproj[13]).getNormalised(),
		Plane3(viewproj[3] - viewproj[2], viewproj[7] - viewproj[6], viewproj[11] - viewproj[10], viewproj[15] - viewproj[14]).getNormalised(),
		Plane3(viewproj[3] + viewproj[2], viewproj[7] + viewproj[6], viewproj[11] + viewproj[10], viewproj[15] + viewproj[14]).getNormalised()
	);
}

inline bool Frustum::testPoint(const Vector3& point) const
{
	return !right.testPoint(point) && !left.testPoint(point) && 
		   !bottom.testPoint(point) && !top.testPoint(point) && 
		   !back.testPoint(point) && !front.testPoint(point);
}

inline bool plane3_test_line(const Plane3& plane, const Segment& segment)
{
	return segment.classifyPlane(plane) == 2; // totally inside
}

inline bool Frustum::testLine(const Segment& segment) const
{
	return !plane3_test_line(right, segment) && 
		   !plane3_test_line(left, segment) && 
		   !plane3_test_line(bottom, segment) && 
		   !plane3_test_line(top, segment) && 
		   !plane3_test_line(back, segment) && 
		   !plane3_test_line(front, segment);
}

/**
 * \brief
 * Operator insertion for Frustum.
 */
inline std::ostream& operator<< (std::ostream& os, const Frustum& frustum)
{
    os << "Frustum { "
       << "left = " << frustum.left << ", "
       << "right = " << frustum.right << ", "
       << "top = " << frustum.top << ", "
       << "bottom = " << frustum.bottom << ", "
       << "front = " << frustum.front << ", "
       << "back = " << frustum.back << " }";
    return os;
}
