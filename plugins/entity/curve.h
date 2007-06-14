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

#if !defined(INCLUDED_CURVE_H)
#define INCLUDED_CURVE_H

/*#include "ientity.h"
#include "selectable.h"
#include "renderable.h"

#include <set>

#include "math/aabb.h"
#include "math/curve.h"
#include "stream/stringstream.h"
#include "signal/signal.h"
#include "selectionlib.h"
#include "render.h"
#include "stringio.h"

class RenderableCurve : public OpenGLRenderable // migrated
{
public:
  std::vector<PointVertex> m_vertices;
  void render(RenderStateFlags state) const
  {
    pointvertex_gl_array(&m_vertices.front());
    glDrawArrays(GL_LINE_STRIP, 0, GLsizei(m_vertices.size()));
  }
};

inline void plotBasisFunction(std::size_t numSegments, int point, int degree)
{
  Knots knots;
  KnotVector_openUniform(knots, 4, degree);

  globalOutputStream() << "plotBasisFunction point " << point << " of 4, knot vector:";
  for(Knots::iterator i = knots.begin(); i != knots.end(); ++i)
  {
    globalOutputStream() << " " << *i;
  }
  globalOutputStream() << "\n";
  globalOutputStream() << "t=0 basis=" << BSpline_basis(knots, point, degree, 0.0) << "\n";
  for(std::size_t i = 1; i < numSegments; ++i)
  {
    double t = (1.0 / double(numSegments)) * double(i);
    globalOutputStream() << "t=" << t << " basis=" << BSpline_basis(knots, point, degree, t) << "\n";
  }
  globalOutputStream() << "t=1 basis=" << BSpline_basis(knots, point, degree, 1.0) << "\n";  
}

inline void ControlPoints_write(const ControlPoints& controlPoints, StringOutputStream& value)
{
  value << Unsigned(controlPoints.size()) << " (";
  for(ControlPoints::const_iterator i = controlPoints.begin(); i != controlPoints.end(); ++i)
  {
    value << " " << (*i).x() << " " << (*i).y() << " " << (*i).z() << " ";
  }
  value << ")";
}

inline void ControlPoint_testSelect(const Vector3& point, ObservedSelectable& selectable, Selector& selector, SelectionTest& test)
{
  SelectionIntersection best;
  test.TestPoint(point, best);
  if(best.valid())
  {
    Selector_add(selector, selectable, best);
  }
}

class ControlPointAddBounds 
{
	AABB& m_bounds;
public:
	ControlPointAddBounds(AABB& bounds) : 
		m_bounds(bounds) 
	{}
	
	void operator()(const Vector3& point, const Vector3& original) const {
		m_bounds.includePoint(point);
	}
};

class ControlPointTransform
{
  const Matrix4& m_matrix;
public:
  ControlPointTransform(const Matrix4& matrix) : m_matrix(matrix)
  {
  }
	void operator()(Vector3& point, const Vector3& original) const {
		// Take the original (untransformed) point and use this as basis
		point = m_matrix.transform(original).getProjected();
	}
};

class ControlPointSnap
{
  float m_snap;
public:
  ControlPointSnap(float snap) : m_snap(snap)
  {
  }
  void operator()(Vector3& point, const Vector3& original) const
  {
    vector3_snap(point, m_snap);
  }
};

const Colour4b colour_vertex(0, 255, 0, 255);
const Colour4b colour_selected(0, 0, 255, 255);

class ControlPointAdd
{
  RenderablePointVector& m_points;
public:
  ControlPointAdd(RenderablePointVector& points) : m_points(points)
  {
  }
  void operator()(const Vector3& point, const Vector3& original) const
  {
    m_points.push_back(PointVertex(Vertex3f(point), colour_vertex));
  }
};

class ControlPointAddSelected
{
  RenderablePointVector& m_points;
public:
  ControlPointAddSelected(RenderablePointVector& points) : m_points(points)
  {
  }
  void operator()(const Vector3& point, const Vector3& original) const
  {
    m_points.push_back(PointVertex(Vertex3f(point), colour_selected));
  }
};

inline void ControlPoints_write(ControlPoints& controlPoints, const char* key, Entity& entity)
{
  StringOutputStream value(256);
  if(!controlPoints.empty())
  {
    ControlPoints_write(controlPoints, value);
  }
  entity.setKeyValue(key, value.c_str());
}*/

#endif
