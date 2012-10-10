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

#if !defined(INCLUDED_VIEW_H)
#define INCLUDED_VIEW_H

#include "ivolumetest.h"
#include "math/Frustum.h"
#include "math/ViewProjection.h"
#include "math/Viewer.h"


#if defined(_DEBUG)
#define DEBUG_CULLING
#endif


#if defined(DEBUG_CULLING)

extern int g_count_dots;
extern int g_count_planes;
extern int g_count_oriented_planes;
extern int g_count_bboxs;
extern int g_count_oriented_bboxs;

#endif

inline void debug_count_dot()
{
#if defined(DEBUG_CULLING)
  ++g_count_dots;
#endif
}

inline void debug_count_plane()
{
#if defined(DEBUG_CULLING)
  ++g_count_planes;
#endif
}

inline void debug_count_oriented_plane()
{
#if defined(DEBUG_CULLING)
  ++g_count_oriented_planes;
#endif
}

inline void debug_count_bbox()
{
#if defined(DEBUG_CULLING)
  ++g_count_bboxs;
#endif
}

inline void debug_count_oriented_bbox()
{
#if defined(DEBUG_CULLING)
  ++g_count_oriented_bboxs;
#endif
}




/// \brief View-volume culling and transformations.
class View : public VolumeTest
{
  /// modelview matrix
  Matrix4 m_modelview;
  /// projection matrix
  Matrix4 m_projection;
  /// device-to-screen transform
  Matrix4 m_viewport;

  Matrix4 m_scissor;

	/// combined modelview and projection matrix
	ViewProjection m_viewproj;

	/// camera position in world space
	Viewer m_viewer;

  /// view frustum in world space
  Frustum m_frustum;

  bool m_fill;

  void construct()
  {
	  m_viewproj = m_scissor.getMultipliedBy(m_projection).getMultipliedBy(m_modelview);

    m_frustum = Frustum::createFromViewproj(m_viewproj);
	m_viewer = Viewer::createFromViewProjection(m_viewproj);
  }
public:
  View(bool fill = false) :
    m_modelview(Matrix4::getIdentity()),
    m_projection(Matrix4::getIdentity()),
    m_scissor(Matrix4::getIdentity()),
    m_fill(fill)
  {
  }
  void Construct(const Matrix4& projection, const Matrix4& modelview, std::size_t width, std::size_t height)
  {
    // modelview
    m_modelview = modelview;

    // projection
    m_projection = projection;

    // viewport
    m_viewport = Matrix4::getIdentity();
    m_viewport[0] = float(width/2);
    m_viewport[5] = float(height/2);
    if(fabs(m_projection[11]) > 0.0000001)
      m_viewport[10] = m_projection[0] * m_viewport[0];
    else
      m_viewport[10] = 1 / m_projection[10];

    construct();
  }
  void EnableScissor(float min_x, float max_x, float min_y, float max_y)
  {
    m_scissor = Matrix4::getIdentity();
    m_scissor[0] = (max_x - min_x) * 0.5f;
    m_scissor[5] = (max_y - min_y) * 0.5f;
    m_scissor[12] = (min_x + max_x) * 0.5f;
    m_scissor[13] = (min_y + max_y) * 0.5f;
    m_scissor.invertFull();

    construct();
  }
  void DisableScissor()
  {
    m_scissor = Matrix4::getIdentity();

    construct();
  }

	bool TestPoint(const Vector3& point) const
	{
		return m_viewproj.testPoint(point);
	}

  bool TestLine(const Segment& segment) const
  {
    return m_frustum.testLine(segment);
  }
  bool TestPlane(const Plane3& plane) const
  {
    debug_count_plane();
	return m_viewer.testPlane(plane);
  }
  bool TestPlane(const Plane3& plane, const Matrix4& localToWorld) const
  {
    debug_count_oriented_plane();
	return m_viewer.testPlane(plane, localToWorld);
  }

    VolumeIntersectionValue TestAABB(const AABB& aabb) const
    {
        debug_count_bbox();
        return m_frustum.testIntersection(aabb);
    }

	VolumeIntersectionValue TestAABB(const AABB& aabb, const Matrix4& localToWorld) const
	{
		debug_count_oriented_bbox();
		return m_frustum.testIntersection(aabb, localToWorld);
	}

  const Matrix4& GetViewMatrix() const
  {
    return m_viewproj;
  }
  const Matrix4& GetViewport() const
  {
    return m_viewport;
  };
  const Matrix4& GetModelview() const
  {
    return m_modelview;
  }
  const Matrix4& GetProjection() const
  {
    return m_projection;
  }

  bool fill() const
  {
    return m_fill;
  }
  const Vector3& getViewer() const
  {
    return m_viewer.getVector3();
  }
};

#endif
