#pragma once

#include <cassert>

/// \file
/// \brief View-frustum data types and related operations.

#include "math/Matrix4.h"
#include "math/Segment.h"
#include "math/AABB.h"

#include "VolumeIntersectionValue.h"

class AABB;
class Plane3;

/**
 * \brief
 * Object representing a frustum, defined by six planes.
 *
 * A frustum is used to represent both the view volume and the volume of a
 * projected light. It typically takes the form of a truncated pyramid although
 * it need not be symmetrical.
 *
 * The six planes defining the frustum are named according to their use with an
 * OpenGL camera: "front" is the near clip plane (the smaller plane near the
 * top of the pyramid) and "back" is the far clip plane (the larger plane at
 * the bottom of the pyramid). In the case of a projected light the "front" is
 * the plane nearest the light source and the "back" is the far end of the
 * light projection.
 */
class Frustum
{
public:
    Plane3 right, left, bottom, top, back, front;

    Frustum()
    {}

    /// Construct a Frustum with six explicit planes
    Frustum(const Plane3& _right, const Plane3& _left,
            const Plane3& _bottom, const Plane3& _top,
            const Plane3& _back, const Plane3& _front)
    : right(_right), left(_left), bottom(_bottom), top(_top),
      back(_back), front(_front)
    {}

    /// Construct the frustum planes from the given projection matrix.
    static Frustum createFromViewproj(const Matrix4& viewproj);

    /// Normalise all planes in the frustum.
    void normalisePlanes();

    /// Get the projection matrix corresponding to the planes of this frustum.
    Matrix4 getProjectionMatrix() const;

    /// Return a copy of this frustum transformed by the given matrix.
    Frustum getTransformedBy(const Matrix4& transform) const;

    /// Test the intersection of this frustum with an AABB.
    VolumeIntersectionValue testIntersection(const AABB& aabb) const;

    /// Test the intersection of this frustum with a transformed AABB.
    VolumeIntersectionValue testIntersection(const AABB& aabb, const Matrix4& localToWorld) const;

    /// Enum representing the corner points of each end plane
    enum Corner
    {
        TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT
    };

    /// Enum representing each end plane
    enum EndPlane
    {
        FRONT, BACK
    };

    /// Return the position of the given corner point
    Vector3 getCornerPoint(EndPlane plane, Corner point) const
    {
        if (plane == FRONT)
        {
            switch (point)
            {
            case TOP_LEFT:
                return Plane3::intersect(left, top, front);
            case TOP_RIGHT:
                return Plane3::intersect(right, top, front);
            case BOTTOM_LEFT:
                return Plane3::intersect(left, bottom, front);
            case BOTTOM_RIGHT:
                return Plane3::intersect(right, bottom, front);
            }
        }
        else
        {
            switch (point)
            {
            case TOP_LEFT:
                return Plane3::intersect(left, top, back);
            case TOP_RIGHT:
                return Plane3::intersect(right, top, back);
            case BOTTOM_LEFT:
                return Plane3::intersect(left, bottom, back);
            case BOTTOM_RIGHT:
                return Plane3::intersect(right, bottom, back);
            }
        }

        // All cases should be handled above
        assert(false);
        return Vector3();
    }

    /// Return an AABB enclosing this frustum
    AABB getAABB() const
    {
        // The AABB of a frustum is simply the AABB which includes all eight
        // corner points.
        AABB result;
        result.includePoint(getCornerPoint(FRONT, TOP_LEFT));
        result.includePoint(getCornerPoint(FRONT, BOTTOM_LEFT));
        result.includePoint(getCornerPoint(FRONT, TOP_RIGHT));
        result.includePoint(getCornerPoint(FRONT, BOTTOM_RIGHT));
        result.includePoint(getCornerPoint(BACK, TOP_LEFT));
        result.includePoint(getCornerPoint(BACK, BOTTOM_LEFT));
        result.includePoint(getCornerPoint(BACK, TOP_RIGHT));
        result.includePoint(getCornerPoint(BACK, BOTTOM_RIGHT));
        return result;
    }

    /// Returns true if the given point is contained in this frustum.
    bool testPoint(const Vector3& point) const;

    bool testLine(const Segment& segment) const;
};

inline Frustum Frustum::createFromViewproj(const Matrix4& viewproj)
{
    // greebo: Note that the usual plane-from-frustum equations which can be found
    // throughout the internet are referring to a plane format a,b,c,d whereas
    // DarkRadiant uses a,b,c and dist, that's why the fourth terms in each line are negated.
	return Frustum
	(
		Plane3(viewproj[3] - viewproj[0], viewproj[7] - viewproj[4], viewproj[11] - viewproj[ 8], -viewproj[15] + viewproj[12]).getNormalised(),
		Plane3(viewproj[3] + viewproj[0], viewproj[7] + viewproj[4], viewproj[11] + viewproj[ 8], -viewproj[15] - viewproj[12]).getNormalised(),
		Plane3(viewproj[3] + viewproj[1], viewproj[7] + viewproj[5], viewproj[11] + viewproj[ 9], -viewproj[15] - viewproj[13]).getNormalised(),
		Plane3(viewproj[3] - viewproj[1], viewproj[7] - viewproj[5], viewproj[11] - viewproj[ 9], -viewproj[15] + viewproj[13]).getNormalised(),
		Plane3(viewproj[3] - viewproj[2], viewproj[7] - viewproj[6], viewproj[11] - viewproj[10], -viewproj[15] + viewproj[14]).getNormalised(),
		Plane3(viewproj[3] + viewproj[2], viewproj[7] + viewproj[6], viewproj[11] + viewproj[10], -viewproj[15] - viewproj[14]).getNormalised()
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
