#pragma once

/// \file
/// \brief Line data types and related operations.

#include "Vector3.h"

/// \brief A line segment defined by a start point and and end point.
class Line
{
public:
	Vector3 start, end;

	Line()
	{}

	Line(const Vector3& start_, const Vector3& end_) : 
		start(start_), 
		end(end_)
	{}

	Vector3 getClosestPoint(const Vector3& point) const
	{
		Vector3 v = end - start;
		Vector3 w = point - start;

		double c1 = w.dot(v);

		if (c1 <= 0)
		{
			return start;
		}

		double c2 = v.dot(v);

		if (c2 <= c1)
		{
			return end;
		}

		return Vector3(start + v * (c1 / c2));
	}
};
