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

// Expand this AABB to include the given point
void AABB::includePoint(const Vector3& point) {
	// Process each axis separately
	for (int i = 0; i < 3; ++i) {
		float axisDisp = point[i] - origin[i]; // axis displacement from origin to point
		float halfDif = 0.5 * (std::abs(axisDisp) - extents[i]); // half of extent increase needed (maybe negative if point inside)
		if (halfDif > 0) {
			origin[i] += (axisDisp > 0) ? halfDif : -halfDif;
			extents[i] += halfDif;
		}
	}
}

