/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "aabb.h"

#include <algorithm>
#include <iostream>


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
			float axisDisp = point[i] - origin[i];

			// Half of extent increase needed (maybe negative if point inside)
			float halfDif = 0.5f * (std::abs(axisDisp) - extents[i]);

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
		    float displacement = other.origin[i] - origin[i];
		    float difference = other.extents[i] - extents[i];

		    if (fabs(displacement) > fabs(difference))
		    {
				float half_difference = 0.5f * (fabs(displacement) + difference);

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
