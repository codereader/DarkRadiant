#pragma once

#include "Vector3.h"
#include "Plane3.h"
#include "Matrix4.h"

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
};
