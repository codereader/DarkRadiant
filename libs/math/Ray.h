#pragma once

#include "Vector3.h"
#include "Plane3.h"
#include "Matrix4.h"
#include "AABB.h"

class Ray
{
public:
	Vector3 origin;
	Vector3 direction;

	Ray()
	{}

	Ray(const Vector3& origin_, const Vector3& direction_) :
		origin(origin_), 
		direction(direction_)
	{}

	static Ray createForPoints(const Vector3& origin, const Vector3& p2)
	{
		return Ray(origin, (p2 - origin).getNormalised());
	}

	/* greebo: this calculates the intersection point of two rays
	 * (copied from Radiant's Intersection code, there may be better ways)
	 */
	Vector3 getIntersection(Ray& other)
	{
		Vector3 intersection = origin - other.origin;

		Vector3::ElementType dot = direction.dot(other.direction);
  		Vector3::ElementType d = direction.dot(intersection);
		Vector3::ElementType e = other.direction.dot(intersection);
		Vector3::ElementType D = 1 - dot*dot;       // always >= 0

		if (D < 0.000001f)
		{
			// the lines are almost parallel
			return other.origin + other.direction*e;
		}
		else
		{
			return other.origin + other.direction*((e - dot*d) / D);
		}
	}

	void transform(const Matrix4& matrix)
	{
		origin = matrix.transformPoint(origin);
		direction = matrix.transformDirection(direction);
	}

	// closest-point-on-line
	Vector3::ElementType getSquaredDistance(const Vector3& point) const
	{
		return (point - (origin + direction * (point - origin).dot(direction))).getLengthSquared();
	}

	Vector3::ElementType getDistance(const Plane3& plane) const
	{
		return -(plane.normal().dot(origin) - plane.dist()) / direction.dot(plane.normal());
	}

	/**
	 * Intersect this ray with the given bounding box. If no intersection occurs, this method
	 * returns FALSE, in case of intersection (even if the ray starts within the AABB volume)
	 * the method returns TRUE. The coord parameter will contain the intersection point in the
	 * latter case (which will be the Ray's origin if it starts within the AABB to test).
	 * Algorithm taken and adjusted from http://tog.acm.org/resources/GraphicsGems/gems/RayBox.c
	 */
	bool intersectAABB(const AABB& aabb, Vector3& intersection) const
	{
		if (!aabb.isValid()) return false;

		#define QUADRANT_RIGHT	0
		#define QUADRANT_LEFT	1
		#define QUADRANT_MIDDLE	2

		bool inside = true;
		char quadrant[3];
		Vector3::ElementType candidatePlane[3];

		Vector3 aabbMin = aabb.getOrigin() - aabb.getExtents();
		Vector3 aabbMax = aabb.getOrigin() + aabb.getExtents();

		// Find candidate planes; this loop can be avoided if
   		// rays cast all from the eye(assume perpsective view) 
		for (int i = 0; i < 3; i++)
		{
			if (origin[i] < aabbMin[i])
			{
				quadrant[i] = QUADRANT_LEFT;
				candidatePlane[i] = aabbMin[i];
				inside = false;
			}
			else if (origin[i] > aabbMax[i])
			{
				quadrant[i] = QUADRANT_RIGHT;
				candidatePlane[i] = aabbMax[i];
				inside = false;
			}
			else
			{
				quadrant[i] = QUADRANT_MIDDLE;
			}
		}

		// Ray origin inside bounding box 
		if (inside)
		{
			intersection = origin;
			return true;
		}

		Vector3::ElementType maxT[3];

		// Calculate T distances to candidate planes
		for (int i = 0; i < 3; i++)
		{
			if (quadrant[i] != QUADRANT_MIDDLE && direction[i] != 0)
			{
				maxT[i] = (candidatePlane[i] - origin[i]) / direction[i];
			}
			else
			{
				maxT[i] = -1;
			}
		}

		// Get largest of the maxT's for final choice of intersection
		int whichPlane = 0;

		for (int i = 1; i < 3; i++)
		{
			if (maxT[whichPlane] < maxT[i])
			{
				whichPlane = i;
			}
		}

		// Check final candidate actually inside box 
		if (maxT[whichPlane] < 0) return false;

		for (int i = 0; i < 3; i++)
		{
			if (whichPlane != i)
			{
				intersection[i] = origin[i] + maxT[whichPlane] * direction[i];

				if (intersection[i] < aabbMin[i] || intersection[i] > aabbMax[i])
				{
					return false;
				}
			} 
			else 
			{
				intersection[i] = candidatePlane[i];
			}
		}

		return true; // ray hits box
	}
};
