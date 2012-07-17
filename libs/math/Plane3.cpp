#include "Plane3.h"

#include "AABB.h"
#include "Matrix4.h"

double Plane3::distanceToOrientedExtents(const Vector3& extents, const Matrix4& orientation) const
{
	return fabs(extents[0] * normal().dot(orientation.x().getVector3())) + 
		   fabs(extents[1] * normal().dot(orientation.y().getVector3())) + 
		   fabs(extents[2] * normal().dot(orientation.z().getVector3()));
}

bool Plane3::containsAABB(const AABB& aabb, const Matrix4& orientation) const
{
	double dot = distanceToPointAABB(aabb.origin);

	return !(dot > 0 || -dot < distanceToOrientedExtents(aabb.extents, orientation));
}
