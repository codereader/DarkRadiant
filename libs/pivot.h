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

#if !defined(INCLUDED_PIVOT_H)
#define INCLUDED_PIVOT_H

#include "math/matrix.h"
#include "generic/static.h"

inline void billboard_viewplaneOriented(Matrix4& rotation, const Matrix4& world2screen)
{
#if 1
  rotation = Matrix4::getIdentity();
  Vector3 x(world2screen.x().getVector3().getNormalised());
  Vector3 y(world2screen.y().getVector3().getNormalised());
  Vector3 z(world2screen.z().getVector3().getNormalised());
  rotation.y().getVector3() = Vector3(x.y(), y.y(), z.y());
  rotation.z().getVector3() = -Vector3(x.z(), y.z(), z.z());
  rotation.x().getVector3() = rotation.y().getVector3().crossProduct(rotation.z().getVector3()).getNormalised();
  rotation.y().getVector3() = rotation.z().getVector3().crossProduct(rotation.x().getVector3());
#else
  Matrix4 screen2world(matrix4_full_inverse(world2screen));

  Vector3 near_(
    matrix4_transformed_vector4(
        screen2world,
        Vector4(0, 0, -1, 1)
      ).getProjected()    
  );

  Vector3 far_(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(0, 0, 1, 1)
      ).getProjected()
  );

  Vector3 up(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(0, 1, -1, 1)
      ).getProjected()
  );

  rotation = Matrix4::getIdentity();
  rotation.y().getVector3() = (up - near_).getNormalised();
  rotation.z().getVector3() = (near_ - far_).getNormalised();
  rotation.x().getVector3() = rotation.y().getVector3().crossProduct(rotation.z().getVector3()).getNormalised();
  rotation.y().getVector3() = rotation.z().getVector3().crossProduct(rotation.x().getVector3());
#endif
}

inline void billboard_viewpointOriented(Matrix4& rotation, const Matrix4& world2screen)
{
  Matrix4 screen2world(matrix4_full_inverse(world2screen));

#if 1
  rotation = Matrix4::getIdentity();
  rotation.y().getVector3() = screen2world.y().getVector3().getNormalised();
  rotation.z().getVector3() = -screen2world.z().getVector3().getNormalised();
  rotation.x().getVector3() = rotation.y().getVector3().crossProduct(rotation.z().getVector3()).getNormalised();
  rotation.y().getVector3() = rotation.z().getVector3().crossProduct(rotation.x().getVector3());
#else
  Vector3 near_(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(world2screen[12] / world2screen[15], world2screen[13] / world2screen[15], -1, 1)
      ).getProjected()
  );

  Vector3 far_(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(world2screen[12] / world2screen[15], world2screen[13] / world2screen[15], 1, 1)
      ).getProjected()
  );

  Vector3 up(
      matrix4_transformed_vector4(
        screen2world,
        Vector4(world2screen[12] / world2screen[15], world2screen[13] / world2screen[15] + 1, -1, 1)
      ).getProjected()
  );

  rotation = Matrix4::getIdentity();
  rotation.y().getVector3() = (up - near_).getNormalised();
  rotation.z().getVector3() = (near_ - far_).getNormalised();
  rotation.x().getVector3() = rotation.y().getVector3().crossProduct(rotation.z().getVector3()).getNormalised();
  rotation.y().getVector3() = rotation.z().getVector3().crossProduct(rotation.x().getVector3());
#endif
}


inline void ConstructObject2Screen(Matrix4& object2screen, const Matrix4& object2world, const Matrix4& world2view, const Matrix4& view2device, const Matrix4& device2screen)
{
  object2screen = device2screen;
  object2screen.multiplyBy(view2device);
  object2screen.multiplyBy(world2view);
  object2screen.multiplyBy(object2world);
}

inline void ConstructObject2Device(Matrix4& object2screen, const Matrix4& object2world, const Matrix4& world2view, const Matrix4& view2device)
{
  object2screen = view2device;
  object2screen.multiplyBy(world2view);
  object2screen.multiplyBy(object2world);
}

inline void ConstructDevice2Object(Matrix4& device2object, const Matrix4& object2world, const Matrix4& world2view, const Matrix4& view2device)
{
  ConstructObject2Device(device2object, object2world, world2view, view2device);
  matrix4_full_invert(device2object);
}

//! S =  ( Inverse(Object2Screen *post ScaleOf(Object2Screen) ) *post Object2Screen
inline void pivot_scale(Matrix4& scale, const Matrix4& pivot2screen)
{
  Matrix4 pre_scale(Matrix4::getIdentity());
  pre_scale[0] = static_cast<float>(pivot2screen.x().getVector3().getLength());
  pre_scale[5] = static_cast<float>(pivot2screen.y().getVector3().getLength());
  pre_scale[10] = static_cast<float>(pivot2screen.z().getVector3().getLength());

  scale = pivot2screen;
  scale.multiplyBy(pre_scale);
  matrix4_full_invert(scale);
  scale.multiplyBy(pivot2screen);
}

// scale by (inverse) W
inline void pivot_perspective(Matrix4& scale, const Matrix4& pivot2screen)
{
  scale = Matrix4::getIdentity();
  scale[0] = scale[5] = scale[10] = pivot2screen[15];
}

inline void ConstructDevice2Manip(Matrix4& device2manip, const Matrix4& object2world, const Matrix4& world2view, const Matrix4& view2device, const Matrix4& device2screen)
{
  Matrix4 pivot2screen;
  ConstructObject2Screen(pivot2screen, object2world, world2view, view2device, device2screen);

  ConstructObject2Device(device2manip, object2world, world2view, view2device);

  Matrix4 scale;
  pivot_scale(scale, pivot2screen);
  device2manip.multiplyBy(scale);
  pivot_perspective(scale, pivot2screen);
  device2manip.multiplyBy(scale);

  matrix4_full_invert(device2manip);
}

inline void Pivot2World_worldSpace(Matrix4& manip2world, const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
{
  manip2world = pivot2world;
  
  Matrix4 pivot2screen;
  ConstructObject2Screen(pivot2screen, pivot2world, modelview, projection, viewport);

  Matrix4 scale;
  pivot_scale(scale, pivot2screen);
  manip2world.multiplyBy(scale);
  pivot_perspective(scale, pivot2screen);
  manip2world.multiplyBy(scale);
}

inline void Pivot2World_viewpointSpace(Matrix4& manip2world, Vector3& axis, const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
{
  manip2world = pivot2world;

  Matrix4 pivot2screen;
  ConstructObject2Screen(pivot2screen, pivot2world, modelview, projection, viewport);

  Matrix4 scale;
  pivot_scale(scale, pivot2screen);
  manip2world.multiplyBy(scale);

  billboard_viewpointOriented(scale, pivot2screen);
  axis = scale.z().getVector3();
  manip2world.multiplyBy(scale);

  pivot_perspective(scale, pivot2screen);
  manip2world.multiplyBy(scale);
}

inline void Pivot2World_viewplaneSpace(Matrix4& manip2world, const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
{
  manip2world = pivot2world;

  Matrix4 pivot2screen;
  ConstructObject2Screen(pivot2screen, pivot2world, modelview, projection, viewport);

  Matrix4 scale;
  pivot_scale(scale, pivot2screen);
  manip2world.multiplyBy(scale);

  billboard_viewplaneOriented(scale, pivot2screen);
  manip2world.multiplyBy(scale);

  pivot_perspective(scale, pivot2screen);
  manip2world.multiplyBy(scale);
}


#include "irenderable.h"
#include "cullable.h"
#include "render.h"

const Colour4b g_colour_x(255, 0, 0, 255);
const Colour4b g_colour_y(0, 255, 0, 255);
const Colour4b g_colour_z(0, 0, 255, 255);

class Shader;

class RenderablePivot : public OpenGLRenderable
{
  VertexBuffer<PointVertex> m_vertices;
  const Vector3& _pivot;
public:
  mutable Matrix4 m_localToWorld;
  typedef Static<ShaderPtr> StaticShader;
  static ShaderPtr getShader()
  {
    return StaticShader::instance();
  }

  RenderablePivot(const Vector3& pivot) :
  	_pivot(pivot)
  {
    m_vertices.reserve(6);

	m_vertices.push_back(PointVertex(_pivot, g_colour_x));
	m_vertices.push_back(PointVertex(_pivot + Vector3(16,0,0), g_colour_x));

	m_vertices.push_back(PointVertex(_pivot, g_colour_y));
	m_vertices.push_back(PointVertex(_pivot + Vector3(0, 16, 0), g_colour_y));

	m_vertices.push_back(PointVertex(_pivot, g_colour_z));
	m_vertices.push_back(PointVertex(_pivot + Vector3(0, 0, 16), g_colour_z));
  }

	/** greebo: Updates the renderable vertex array to the given pivot point 
	 */
	void updatePivot() {
		m_vertices.clear();
		
		m_vertices.push_back(PointVertex(_pivot, g_colour_x));
		m_vertices.push_back(PointVertex(_pivot + Vector3(16,0,0), g_colour_x));

		m_vertices.push_back(PointVertex(_pivot, g_colour_y));
		m_vertices.push_back(PointVertex(_pivot + Vector3(0, 16, 0), g_colour_y));

		m_vertices.push_back(PointVertex(_pivot, g_colour_z));
		m_vertices.push_back(PointVertex(_pivot + Vector3(0, 0, 16), g_colour_z));
	}

  void render(const RenderInfo& info) const
  {
    if(m_vertices.size() == 0) return;
    if(m_vertices.data() == 0) return;
    glVertexPointer(3, GL_DOUBLE, sizeof(PointVertex), &m_vertices.data()->vertex);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &m_vertices.data()->colour);
    glDrawArrays(GL_LINES, 0, m_vertices.size());
  }

  void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    collector.PushState();

	// greebo: Commented this out to avoid the point from being moved along with the view.
    //Pivot2World_worldSpace(m_localToWorld, localToWorld, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    collector.Highlight(RenderableCollector::ePrimitive, false);
    collector.SetState(getShader(), RenderableCollector::eWireframeOnly);
    collector.SetState(getShader(), RenderableCollector::eFullMaterials);
    collector.addRenderable(*this, localToWorld);

    collector.PopState();
  }
};



#endif
