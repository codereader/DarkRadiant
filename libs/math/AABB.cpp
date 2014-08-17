#include "AABB.h"

#include "Plane3.h"
#include "Matrix4.h"

// Expand this AABB to include the given point.
void AABB::includePoint(const Vector3& point) {

	// If not yet initialised, simply set the AABB to equal the point
	if (!isValid())
	{
		origin = point;
		extents = Vector3(0, 0, 0);
	}
	else
	{
		// Extend each axis separately
		for (int i = 0; i < 3; ++i)
		{
			// Axis displacement from origin to point
			double axisDisp = point[i] - origin[i];

			// Half of extent increase needed (maybe negative if point inside)
			double halfDif = 0.5f * (std::abs(axisDisp) - extents[i]);

			if (halfDif > 0)
			{
				origin[i] += (axisDisp > 0) ? halfDif : -halfDif;
				extents[i] += halfDif;
			}
		}
	}
}

// Expand this AABB to include another AABB
void AABB::includeAABB(const AABB& other)
{
	// Validity check. If both this and other are valid, use the extension
	// algorithm. If only the other AABB is valid, set this AABB equal to it.
	// If neither are valid we do nothing.

	if (isValid() && other.isValid())
	{
		// Extend each axis separately
		for (int i = 0; i < 3; ++i)
		{
		    double displacement = other.origin[i] - origin[i];
		    double difference = other.extents[i] - extents[i];

		    if (fabs(displacement) > fabs(difference))
		    {
				double half_difference = 0.5f * (fabs(displacement) + difference);

				if (half_difference > 0.0f)
				{
					origin[i] += (displacement >= 0.0f) ? half_difference : -half_difference;
					extents[i] += half_difference;
				}
		    }
		    else if (difference > 0.0f)
		    {
				origin[i] = other.origin[i];
				extents[i] = other.extents[i];
		    }
		}
	}
	else if (other.isValid())
	{
		origin = other.origin;
		extents = other.extents;
	}
}

VolumeIntersectionValue AABB::classifyPlane(const Plane3& plane) const
{
    double originDot = plane.normal().dot(origin);
    double extentsDot = fabs(plane.normal().x()) * extents.x() +
        fabs(plane.normal().y()) * extents.y() +
        fabs(plane.normal().z()) * extents.z();

    if (originDot + extentsDot - plane.dist() < 0)
    {
        // largest dot product is still behind the plane
        return VOLUME_OUTSIDE;
    }

    if (originDot - extentsDot - plane.dist() >= 0)
    {
        // smallest dot product still in front of the plane
        return VOLUME_INSIDE;
    }

    return VOLUME_PARTIAL;

#if 0
    // greebo: I've adjusted this code (as the old one was very likely wrong)
    // following the explanations on AABB vs. Frustum intersection tests
    // found here: http://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/

    double originDot = plane.normal().dot(origin);
    double extendsDot = fabs(plane.normal().x()) * extents[0] +
        fabs(plane.normal().y()) * extents[1] +
        fabs(plane.normal().z()) * extents[2];

    if (originDot + extendsDot > plane.dist())
    {
        // At least one point is on the positive side of the plane
        return VOLUME_PARTIAL; // partially inside
    }
    else if (originDot - extendsDot >= plane.dist())
    {
        // Even the minimum point is on the positive side
        return VOLUME_INSIDE; // totally inside
    }

    return VOLUME_OUTSIDE; // totally outside
#endif
}

unsigned int AABB::classifyOrientedPlane(const Matrix4& transform, const Plane3& plane) const
{
	double distance_origin = plane.normal().dot(origin) + plane.dist();

	if (fabs(distance_origin) < (fabs(extents[0] * plane.normal().dot(transform.x().getVector3())) + 
								 fabs(extents[1] * plane.normal().dot(transform.y().getVector3())) + 
								 fabs(extents[2] * plane.normal().dot(transform.z().getVector3()))))
	{
		return 1; // partially inside
	}
	else if (distance_origin < 0)
	{
		return 2; // totally inside
	}
	
	return 0; // totally outside
}

void AABB::getCorners(Vector3 corners[8], const Matrix4& rotation) const
{
	Vector3 x = rotation.x().getVector3() * extents.x();
	Vector3 y = rotation.y().getVector3() * extents.y();
	Vector3 z = rotation.z().getVector3() * extents.z();

	corners[0] = origin - x + y + z;
	corners[1] = origin + x + y + z;
	corners[2] = origin + x - y + z;
	corners[3] = origin - x - y + z;
	corners[4] = origin - x + y - z;
	corners[5] = origin + x + y - z;
	corners[6] = origin + x - y - z;
	corners[7] = origin - x - y - z;
}

void AABB::getPlanes(Plane3 planes[6], const Matrix4& rotation) const
{
	double x = rotation.x().getVector3().dot(origin);
	double y = rotation.y().getVector3().dot(origin);
	double z = rotation.z().getVector3().dot(origin);

	planes[0] = Plane3( rotation.x().getVector3(), x + extents[0]);
	planes[1] = Plane3(-rotation.x().getVector3(), -(x - extents[0]));
	planes[2] = Plane3( rotation.y().getVector3(), y + extents[1]);
	planes[3] = Plane3(-rotation.y().getVector3(), -(y - extents[1]));
	planes[4] = Plane3( rotation.z().getVector3(), z + extents[2]);
	planes[5] = Plane3(-rotation.z().getVector3(), -(z - extents[2]));
}

AABB AABB::createFromOrientedAABB(const AABB& aabb, const Matrix4& transform)
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
