#pragma once

#include "Vector3.h"
#include "Plane3.h"

class Segment
{
public:
	Vector3 origin;
	Vector3 extents;

	Segment()
	{}

	Segment(const Vector3& origin_, const Vector3& extents_) :
		origin(origin_), 
		extents(extents_)
	{}

	static Segment createForStartEnd(const Vector3& start, const Vector3& end)
	{
		Segment segment;
		
		segment.origin = start.mid(end);
		segment.extents = end - segment.origin;

		return segment;
	}

	unsigned int classifyPlane(const Plane3& plane) const
	{
		double distance_origin = plane.normal().dot(origin) + plane.dist();

		if (fabs(distance_origin) < fabs(plane.normal().dot(extents)))
		{
			return 1; // partially inside
		}
		else if (distance_origin < 0)
		{
			return 2; // totally inside
		}

		return 0; // totally outside
	}
};
