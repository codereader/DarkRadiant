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

#include <GL/glew.h>

#include <algorithm>
#include <iostream>

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

// OpenGL render function

void AABB::render(RenderStateFlags flags) const {

	// Wireframe cuboid
	glBegin(GL_LINES);
		glVertex3f(extents.x(), extents.y(), extents.z());
		glVertex3f(extents.x(), extents.y(), -extents.z());

		glVertex3f(extents.x(), extents.y(), extents.z());
		glVertex3f(-extents.x(), extents.y(), extents.z());

		glVertex3f(extents.x(), extents.y(), -extents.z());
		glVertex3f(-extents.x(), extents.y(), -extents.z());

		glVertex3f(extents.x(), extents.y(), extents.z());
		glVertex3f(extents.x(), -extents.y(), extents.z());

		glVertex3f(-extents.x(), extents.y(), extents.z());
		glVertex3f(-extents.x(), -extents.y(), extents.z());

		glVertex3f(-extents.x(), extents.y(), -extents.z());
		glVertex3f(-extents.x(), -extents.y(), -extents.z());

		glVertex3f(extents.x(), extents.y(), -extents.z());
		glVertex3f(extents.x(), -extents.y(), -extents.z());

		glVertex3f(extents.x(), -extents.y(), extents.z());
		glVertex3f(-extents.x(), -extents.y(), extents.z());

		glVertex3f(extents.x(), -extents.y(), extents.z());
		glVertex3f(extents.x(), -extents.y(), -extents.z());

		glVertex3f(-extents.x(), extents.y(), extents.z());
		glVertex3f(-extents.x(), extents.y(), -extents.z());

		glVertex3f(-extents.x(), -extents.y(), extents.z());
		glVertex3f(-extents.x(), -extents.y(), -extents.z());

		glVertex3f(extents.x(), -extents.y(), -extents.z());
		glVertex3f(-extents.x(), -extents.y(), -extents.z());
	glEnd();
}
