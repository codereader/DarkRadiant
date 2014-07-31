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
    // greebo: DR's Plane3 is seriuosly hambered by its internal representation
    // which is nx,ny,nz,dist instead of a,b,c,d. This causes a lot of confusion
    // and makes it necessary to invert the dist() member each time before
    // applying a transformation matrix.

    Plane3 rightTemp = Plane3(right.normal(), -right.dist()).transform(matrix);
    Plane3 leftTemp = Plane3(left.normal(), -left.dist()).transform(matrix);
    Plane3 topTemp = Plane3(top.normal(), -top.dist()).transform(matrix);
    Plane3 bottomTemp = Plane3(bottom.normal(), -bottom.dist()).transform(matrix);
    Plane3 backTemp = Plane3(back.normal(), -back.dist()).transform(matrix);
    Plane3 frontTemp = Plane3(front.normal(), -front.dist()).transform(matrix);

    rightTemp.dist() = -rightTemp.dist();
    leftTemp.dist() = -leftTemp.dist();
    topTemp.dist() = -topTemp.dist();
    bottomTemp.dist() = -bottomTemp.dist();
    backTemp.dist() = -backTemp.dist();
    frontTemp.dist() = -frontTemp.dist();

    return Frustum(
        rightTemp,
        leftTemp,
        bottomTemp,
        topTemp,
        backTemp,
        frontTemp
    );
}

// Test intersection with an AABB
VolumeIntersectionValue Frustum::testIntersection(const AABB& aabb) const
{
    VolumeIntersectionValue result = VOLUME_INSIDE;

	switch (aabb.classifyPlane(right))
    {
    case VOLUME_OUTSIDE:
      return VOLUME_OUTSIDE;
    case VOLUME_PARTIAL:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(left))
    {
    case VOLUME_OUTSIDE:
      return VOLUME_OUTSIDE;
    case VOLUME_PARTIAL:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(bottom))
    {
    case VOLUME_OUTSIDE:
      return VOLUME_OUTSIDE;
    case VOLUME_PARTIAL:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(top))
    {
    case VOLUME_OUTSIDE:
      return VOLUME_OUTSIDE;
    case VOLUME_PARTIAL:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(back))
    {
    case VOLUME_OUTSIDE:
      return VOLUME_OUTSIDE;
    case VOLUME_PARTIAL:
      result = VOLUME_PARTIAL;
    }

	switch (aabb.classifyPlane(front))
    {
    case VOLUME_OUTSIDE:
      return VOLUME_OUTSIDE;
    case VOLUME_PARTIAL:
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
