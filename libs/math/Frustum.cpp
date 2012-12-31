#include "Frustum.h"

#include "AABB.h"

// Normalise all planes in frustum
void Frustum::normalisePlanes()
{
    left = left.getNormalised();
    right = right.getNormalised();
    top = top.getNormalised();
    bottom = bottom.getNormalised();
    back = back.getNormalised();
    front = front.getNormalised();
}

// Get a projection matrix from the frustum
Matrix4 Frustum::getProjectionMatrix() const
{
    return Matrix4::byColumns(
        // col 1
        (right.normal().x() - left.normal().x()) / 2,
        (top.normal().x() - bottom.normal().x()) / 2,
        (back.normal().x() - front.normal().x()) / 2,
        right.normal().x() - (right.normal().x() - left.normal().x()) / 2,
        // col 2
        (right.normal().y() - left.normal().y()) / 2,
        (top.normal().y() - bottom.normal().y()) / 2,
        (back.normal().y() - front.normal().y()) / 2,
        right.normal().y() - (right.normal().y() - left.normal().y()) / 2,
        // col 3
        (right.normal().z() - left.normal().z()) / 2,
        (top.normal().z() - bottom.normal().z()) / 2,
        (back.normal().z() - front.normal().z()) / 2,
        right.normal().z() - (right.normal().z() - left.normal().z()) / 2,
        // col 4
        (right.dist() - left.dist()) / 2,
        (top.dist() - bottom.dist()) / 2,
        (back.dist() - front.dist()) / 2,
        right.dist() - (right.dist() - left.dist()) / 2
    );
}

// Get a transformed copy of this frustum
Frustum Frustum::getTransformedBy(const Matrix4& matrix) const
{
    return Frustum(
        right.transformed(matrix),
        left.transformed(matrix),
        bottom.transformed(matrix),
        top.transformed(matrix),
        back.transformed(matrix),
        front.transformed(matrix)
    );
}

// Test intersection with an AABB
VolumeIntersectionValue Frustum::testIntersection(const AABB& aabb) const
{
    VolumeIntersectionValue result = VOLUME_INSIDE;

	switch (aabb.classifyPlane(right))
    {
    case 2:
      return VOLUME_OUTSIDE;
    case 1:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(left))
    {
    case 2:
      return VOLUME_OUTSIDE;
    case 1:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(bottom))
    {
    case 2:
      return VOLUME_OUTSIDE;
    case 1:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(top))
    {
    case 2:
      return VOLUME_OUTSIDE;
    case 1:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(back))
    {
    case 2:
      return VOLUME_OUTSIDE;
    case 1:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(front))
    {
    case 2:
      return VOLUME_OUTSIDE;
    case 1:
      result = VOLUME_PARTIAL;
    }

    return result;
}

VolumeIntersectionValue Frustum::testIntersection(const AABB& aabb, const Matrix4& localToWorld) const
{
	AABB aabb_world(aabb);
	aabb_world.origin = localToWorld.transformPoint(aabb_world.origin);

	if (right.containsAABB(aabb_world, localToWorld) || 
		left.containsAABB(aabb_world, localToWorld) || 
		bottom.containsAABB(aabb_world, localToWorld) || 
		top.containsAABB(aabb_world, localToWorld) || 
		back.containsAABB(aabb_world, localToWorld) || 
		front.containsAABB(aabb_world, localToWorld))
	{
		return VOLUME_OUTSIDE;
	}

	return VOLUME_INSIDE;
}
