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

#include "View.h"

#if defined(_DEBUG)
#define DEBUG_CULLING
#endif

#if defined(DEBUG_CULLING)

#include <stdio.h>

char g_cull_stats[1024];
int g_count_dots;
int g_count_planes;
int g_count_oriented_planes;
int g_count_bboxs;
int g_count_oriented_bboxs;

#endif

void Cull_ResetStats()
{
#if defined(DEBUG_CULLING)
  g_count_dots = 0;
  g_count_planes = 0;
  g_count_oriented_planes = 0;
  g_count_bboxs = 0;
  g_count_oriented_bboxs = 0;
#endif
}

const char* Cull_GetStats()
{
#if defined(DEBUG_CULLING)
  sprintf(g_cull_stats, "dots: %d | planes %d + %d | bboxs %d + %d", g_count_dots, g_count_planes, g_count_oriented_planes, g_count_bboxs, g_count_oriented_bboxs);
  return g_cull_stats;
#else
  return "";
#endif
}

#if defined(DEBUG_CULLING)
#define INC_COUNTER(x) (++(x))
#else
#define INC_COUNTER(x)
#endif

View::View(bool fill) :
	_modelview(Matrix4::getIdentity()),
	_projection(Matrix4::getIdentity()),
	_scissor(Matrix4::getIdentity()),
	_fill(fill)
{}

void View::Construct(const Matrix4& projection, const Matrix4& modelview, std::size_t width, std::size_t height)
{
	// modelview
	_modelview = modelview;

	// projection
	_projection = projection;

	// viewport
	_viewport = Matrix4::getIdentity();
	_viewport[0] = float(width/2);
	_viewport[5] = float(height/2);
	if(fabs(_projection[11]) > 0.0000001)
		_viewport[10] = _projection[0] * _viewport[0];
	else
		_viewport[10] = 1 / _projection[10];

	construct();
}

void View::EnableScissor(float min_x, float max_x, float min_y, float max_y)
{
	_scissor = Matrix4::getIdentity();
	_scissor[0] = (max_x - min_x) * 0.5f;
	_scissor[5] = (max_y - min_y) * 0.5f;
	_scissor[12] = (min_x + max_x) * 0.5f;
	_scissor[13] = (min_y + max_y) * 0.5f;
	_scissor.invertFull();

	construct();
}

void View::DisableScissor()
{
	_scissor = Matrix4::getIdentity();

	construct();
}

bool View::TestPoint(const Vector3& point) const
{
	return _viewproj.testPoint(point);
}

bool View::TestLine(const Segment& segment) const
{
	return _frustum.testLine(segment);
}

bool View::TestPlane(const Plane3& plane) const
{
	INC_COUNTER(g_count_planes);
	return _viewer.testPlane(plane);
}

bool View::TestPlane(const Plane3& plane, const Matrix4& localToWorld) const
{
	INC_COUNTER(g_count_oriented_planes);
	return _viewer.testPlane(plane, localToWorld);
}

VolumeIntersectionValue View::TestAABB(const AABB& aabb) const
{
	INC_COUNTER(g_count_bboxs);
    return _frustum.testIntersection(aabb);
}

VolumeIntersectionValue View::TestAABB(const AABB& aabb, const Matrix4& localToWorld) const
{
	INC_COUNTER(g_count_oriented_bboxs);
	return _frustum.testIntersection(aabb, localToWorld);
}

const Matrix4& View::GetViewMatrix() const
{
	return _viewproj;
}

const Matrix4& View::GetViewport() const
{
	return _viewport;
}

const Matrix4& View::GetModelview() const
{
	return _modelview;
}

const Matrix4& View::GetProjection() const
{
	return _projection;
}

bool View::fill() const
{
	return _fill;
}

const Vector3& View::getViewer() const
{
	return _viewer.getVector3();
}

void View::construct()
{
	_viewproj = _scissor.getMultipliedBy(_projection).getMultipliedBy(_modelview);

	_frustum = Frustum::createFromViewproj(_viewproj);
	_viewer = Viewer::createFromViewProjection(_viewproj);
}
