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

void Plane3::translate(const Vector3& translation)
{
    _dist = _dist - ( translation.x() * _normal.x()
                    + translation.y() * _normal.y()
                    + translation.z() * _normal.z());
}

Plane3& Plane3::transform(const Matrix4& m)
{
    _normal = m.transformDirection(_normal);

    _dist = _normal.x() * (_dist * _normal.x() - m.tx())
          + _normal.y() * (_dist * _normal.y() - m.ty())
          + _normal.z() * (_dist * _normal.z() - m.tz());

    return *this;
}

Plane3 Plane3::transformed(const Matrix4& m) const
{
    return Plane3(*this).transform(m);
}
