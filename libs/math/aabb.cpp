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

// Named constructor to create an AABB from the given minimum and maximum
// points.
AABB AABB::createFromMinMax(const Vector3& min, const Vector3& max) {

	// Origin is the midpoint of the two vectors
	Vector3 origin = (min + max) * 0.5;
	
	// Extents is the vector from the origin to the max point
	Vector3 extents = max - origin;
	
	// Construct and return the resulting AABB;
	return AABB(origin, extents);
}

// Check whether the AABB is valid, or if the extents are still uninitialised
bool AABB::isValid() const {

	bool valid = true;

	// Check each origin and extents value. The origins must be between
	// +/- FLT_MAX, and the extents between 0 and FLT_MAX.
	for (int i = 0; i < 3; ++i) {
		if (origin[i] < -FLT_MAX
			|| origin[i] > FLT_MAX
			|| extents[i] < 0
			|| extents[i] > FLT_MAX)
		{
			valid = false;
		}
	}

	return valid;
}

// Expand this AABB to include the given point.
void AABB::includePoint(const Vector3& point) {

	// If not yet initialised, simply set the AABB to equal the point
	if (!isValid()) {
		origin = point;
		extents = Vector3(0, 0, 0);
	}
	else {
		// Extend each axis separately
		for (int i = 0; i < 3; ++i) {
			// Axis displacement from origin to point
			double axisDisp = point[i] - origin[i]; 
			// Half of extent increase needed (maybe negative if point inside)
			double halfDif = 0.5 * (std::abs(axisDisp) - extents[i]); 
			if (halfDif > 0) {
				origin[i] += (axisDisp > 0) ? halfDif : -halfDif;
				extents[i] += halfDif;
			}
		}
	}
}

// Expand this AABB to include another AABB
void AABB::includeAABB(const AABB& other) {
	
	// Validity check. If both this and other are valid, use the extension
	// algorithm. If only the other AABB is valid, set this AABB equal to it.
	// If neither are valid we do nothing.

	if (isValid() && other.isValid()) {
		// Extend each axis separately
		for (int i = 0; i < 3; ++i) {
		    double displacement = other.origin[i] - origin[i];
		    double difference = other.extents[i] - extents[i];
		    if(fabs(displacement) > fabs(difference))
		    {
		      double half_difference = 0.5 * (fabs(displacement) + difference);
		      if(half_difference > 0.0f)
		      {
		        origin[i] += (displacement >= 0.0f) ? half_difference : -half_difference;
		        extents[i] += half_difference;
		      }
		    }
		    else if(difference > 0.0f)
		    {
		      origin[i] = other.origin[i];
		      extents[i] = other.extents[i];
		    }
		}
	}
	else if (other.isValid()) {
		origin = other.origin;
		extents = other.extents;	
	}
}
