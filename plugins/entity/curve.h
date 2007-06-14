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

#include "ientity.h"
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

inline bool ControlPoints_parse(ControlPoints& controlPoints, const char* value) // migrated
{
  StringTokeniser tokeniser(value, " ");

  std::size_t size;
  if(!string_parse_size(tokeniser.getToken(), size))
  {
    return false;
  }

  if(size < 3)
  {
    return false;
  }
  controlPoints.resize(size);

  if(!string_equal(tokeniser.getToken(), "("))
  {
    return false;
  }
  for(ControlPoints::iterator i = controlPoints.begin(); i != controlPoints.end(); ++i)
  {
    if(!string_parse_double(tokeniser.getToken(), (*i).x())
      || !string_parse_double(tokeniser.getToken(), (*i).y())
      || !string_parse_double(tokeniser.getToken(), (*i).z()))
    {
      return false;
    }
  }
  if(!string_equal(tokeniser.getToken(), ")"))
  {
    return false;
  }
  return true;
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

class CurveEditType
{
public:
  ShaderPtr m_controlsShader;
  ShaderPtr m_selectedShader;
};

inline void ControlPoints_write(ControlPoints& controlPoints, const char* key, Entity& entity)
{
  StringOutputStream value(256);
  if(!controlPoints.empty())
  {
    ControlPoints_write(controlPoints, value);
  }
  entity.setKeyValue(key, value.c_str());
}

class CurveEdit
{
  SelectionChangeCallback m_selectionChanged;
  ControlPoints& m_controlPointsTransformed;
  const ControlPoints& m_controlPoints;
  typedef Array<ObservedSelectable> Selectables;
  Selectables m_selectables;

  RenderablePointVector m_controlsRender;
  mutable RenderablePointVector m_selectedRender;

public:
  typedef Static<CurveEditType> Type;

  CurveEdit(ControlPoints& controlPointsTransformed, //  The working set
  			const ControlPoints& controlPoints,	// the unchanged reference control points 
  			const SelectionChangeCallback& selectionChanged) :
    m_selectionChanged(selectionChanged),
    m_controlPointsTransformed(controlPointsTransformed),
    m_controlPoints(controlPoints),
    m_controlsRender(GL_POINTS),
    m_selectedRender(GL_POINTS)
  {
  }

	template<typename Functor>
	const Functor& forEachSelected(const Functor& functor) {
		ASSERT_MESSAGE(m_controlPointsTransformed.size() == m_selectables.size(), "curve instance mismatch");
		ControlPoints::iterator transformed = m_controlPointsTransformed.begin();
		ControlPoints::const_iterator original = m_controlPoints.begin();
    
		for (Selectables::iterator i = m_selectables.begin(); 
			 i != m_selectables.end(); 
			 ++i, ++transformed, ++original)
		{
			if (i->isSelected()) {
        		functor(*transformed, *original);
      		}
		}
		return functor;
	}

	template<typename Functor>
	const Functor& forEachSelected(const Functor& functor) const {
		ASSERT_MESSAGE(m_controlPointsTransformed.size() == m_selectables.size(), "curve instance mismatch");
		ControlPoints::const_iterator transformed = m_controlPointsTransformed.begin();
		ControlPoints::const_iterator original = m_controlPoints.begin();
    
		for (Selectables::const_iterator i = m_selectables.begin(); 
			 i != m_selectables.end(); 
			 ++i, ++transformed, ++original)
		{
			if (i->isSelected()) {
        		functor(*transformed, *original);
      		}
		}
		return functor;
	}
	
  template<typename Functor>
  const Functor& forEach(const Functor& functor)
  {
  	ControlPoints::const_iterator original = m_controlPoints.begin();
    for(ControlPoints::iterator i = m_controlPointsTransformed.begin(); 
    	i != m_controlPointsTransformed.end(); 
    	++i, ++original)
    {
      functor(*i, *original);
    }
    return functor;
  }
	
  template<typename Functor>
  const Functor& forEach(const Functor& functor) const
  {
  	ControlPoints::const_iterator original = m_controlPoints.begin();
    for(ControlPoints::const_iterator i = m_controlPointsTransformed.begin(); 
    	i != m_controlPointsTransformed.end(); 
    	++i, ++original)
    {
      functor(*i, *original);
    }
    return functor;
  }

  void testSelect(Selector& selector, SelectionTest& test)
  {
    ASSERT_MESSAGE(m_controlPointsTransformed.size() == m_selectables.size(), "curve instance mismatch");
    ControlPoints::const_iterator p = m_controlPointsTransformed.begin();
    for(Selectables::iterator i = m_selectables.begin(); i != m_selectables.end(); ++i, ++p)
    {
      ControlPoint_testSelect(*p, *i, selector, test);
    }
  }

  bool isSelected() const
  {
    for(Selectables::const_iterator i = m_selectables.begin(); i != m_selectables.end(); ++i)
    {
      if((*i).isSelected())
      {
        return true;
      }
    }
    return false;
  }
  void setSelected(bool selected)
  {
    for(Selectables::iterator i = m_selectables.begin(); i != m_selectables.end(); ++i)
    {
      (*i).setSelected(selected);
    }
  }

  void write(const char* key, Entity& entity)
  {
    ControlPoints_write(m_controlPointsTransformed, key, entity);
  }

  void transform(const Matrix4& matrix, bool selectedOnly = true)
  {
  	if (selectedOnly) {
    	forEachSelected(ControlPointTransform(matrix));
  	}
  	else {
  		forEach(ControlPointTransform(matrix));
  	}
  }
  void snapto(float snap)
  {
    forEachSelected(ControlPointSnap(snap));
  }

  void updateSelected() const
  {
    m_selectedRender.clear();
    forEachSelected(ControlPointAddSelected(m_selectedRender));
  }
  
  void renderComponents(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    renderer.SetState(Type::instance().m_controlsShader, Renderer::eWireframeOnly);
    renderer.SetState(Type::instance().m_controlsShader, Renderer::eFullMaterials);
    renderer.addRenderable(m_controlsRender, localToWorld);
  }

  void renderComponentsSelected(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    updateSelected();
    if(!m_selectedRender.empty())
    {
      renderer.Highlight(Renderer::ePrimitive, false);
      renderer.SetState(Type::instance().m_selectedShader, Renderer::eWireframeOnly);
      renderer.SetState(Type::instance().m_selectedShader, Renderer::eFullMaterials);
      renderer.addRenderable(m_selectedRender, localToWorld);
    }
  }

  void curveChanged()
  {
    m_selectables.resize(m_controlPointsTransformed.size(), m_selectionChanged);

    m_controlsRender.clear();
    m_controlsRender.reserve(m_controlPointsTransformed.size());
    forEach(ControlPointAdd(m_controlsRender));

    m_selectedRender.reserve(m_controlPointsTransformed.size());
  }
  typedef MemberCaller<CurveEdit, &CurveEdit::curveChanged> CurveChangedCaller;
};



/*const int NURBS_degree = 3;

class NURBSCurve
{
  Signal0 m_curveChanged;	// migrated
  Callback m_boundsChanged;	// migrated
public:
  ControlPoints m_controlPoints;	// migrated
  ControlPoints m_controlPointsTransformed;	// migrated
  NURBSWeights m_weights;	// migrated
  Knots m_knots;	// migrated
  RenderableCurve m_renderCurve;	// migrated
  AABB m_bounds;	// migrated

  NURBSCurve(const Callback& boundsChanged) : m_boundsChanged(boundsChanged) // migrated
  {
  }

  SignalHandlerId connect(const SignalHandler& curveChanged) // migrated
  {
    curveChanged();
    return m_curveChanged.connectLast(curveChanged);
  }
  void disconnect(SignalHandlerId id) // migrated
  {
    m_curveChanged.disconnect(id);
  }
  void notify() // migrated
  {
    m_curveChanged();
  }

  void tesselate() // migrated
  {
    if(!m_controlPointsTransformed.empty())
    {
      const std::size_t numSegments = (m_controlPointsTransformed.size() - 1) * 16;
      m_renderCurve.m_vertices.resize(numSegments + 1);
      m_renderCurve.m_vertices[0].vertex = Vertex3f(m_controlPointsTransformed[0]);
      for(std::size_t i = 1; i < numSegments; ++i)
      {
        m_renderCurve.m_vertices[i].vertex = Vertex3f(NURBS_evaluate(m_controlPointsTransformed, m_weights, m_knots, NURBS_degree, (1.0 / double(numSegments)) * double(i)));
      }
      m_renderCurve.m_vertices[numSegments].vertex = Vertex3f(m_controlPointsTransformed[m_controlPointsTransformed.size() - 1]);
    }
    else
    {
      m_renderCurve.m_vertices.clear();
    }
  }

  void curveChanged()	// migrated
  {
    tesselate();

    m_bounds = AABB();
    for(ControlPoints::iterator i = m_controlPointsTransformed.begin(); i != m_controlPointsTransformed.end(); ++i)
    {
      m_bounds.includePoint(*i);
    }

    m_boundsChanged();
    notify();
  }

	void appendControlPoints(unsigned int numPoints) {	// migrated
		std::size_t size = m_controlPoints.size();
		
		if (size < 1) {
			return;
		}
		
		// The coordinates of the penultimate point (can be 0,0,0)
		Vector3 penultimate = (size > 1) ? m_controlPoints[size - 2] : Vector3(0,0,0);
		Vector3 ultimate = m_controlPoints[size - 1];
		
		// Calculate the extrapolation vector
		Vector3 extrapolation = ultimate - penultimate;
		
		// greebo: TODO: Use std::vector for this instead of that crappy Array
		ControlPoints newPoints;
		newPoints.resize(size+1);
		
		// Copy the points from the source set
		ControlPoints::iterator target = newPoints.begin();
		for(ControlPoints::iterator source = m_controlPoints.begin(); 
			source != m_controlPoints.end(); 
			++source, ++target)
		{
			*target = *source; 
		}
		
		// The iterator points now to the last element, fill it
		*target = ultimate + extrapolation;
		
		// Replace the old control points with the larger set
		m_controlPoints = newPoints;
		
		// Update the transformation working set
		m_controlPointsTransformed = m_controlPoints;
		
		// Do the maths
		doWeighting();
		curveChanged();
	}
	
	void doWeighting() {	// migrated
		// Re-adjust the weights
		m_weights.resize(m_controlPoints.size());
		for(NURBSWeights::iterator i = m_weights.begin(); i != m_weights.end(); ++i) {
			(*i) = 1;
		}
		
		// greebo: Recalculate the knots (?)
		KnotVector_openUniform(m_knots, m_controlPoints.size(), NURBS_degree);
	}

  bool parseCurve(const char* value)	// migrated
  {
    if(!ControlPoints_parse(m_controlPoints, value))
    {
      return false;
    }

	doWeighting();

    //plotBasisFunction(8, 0, NURBS_degree);

    return true;
  }

  void curveChanged(const std::string& value) // migrated
  {
    if(value.empty() || !parseCurve(value.c_str()))
    {
      m_controlPoints.resize(0);
      m_knots.resize(0);
      m_weights.resize(0);
    }
    m_controlPointsTransformed = m_controlPoints;
    curveChanged();
  }
  typedef MemberCaller1<NURBSCurve, const std::string&, &NURBSCurve::curveChanged> CurveChangedCaller;
};*/

/*class CatmullRomSpline
{
  Signal0 m_curveChanged;	// migrated
  Callback m_boundsChanged; // migrated
public:
  ControlPoints m_controlPoints; // migrated
  ControlPoints m_controlPointsTransformed; // migrated
  RenderableCurve m_renderCurve; // migrated
  AABB m_bounds; // migrated

  CatmullRomSpline(const Callback& boundsChanged) : m_boundsChanged(boundsChanged)// migrated
  {
  }

  SignalHandlerId connect(const SignalHandler& curveChanged)// migrated
  {
    curveChanged();
    return m_curveChanged.connectLast(curveChanged);
  }
  void disconnect(SignalHandlerId id)// migrated
  {
    m_curveChanged.disconnect(id);
  }
  void notify()// migrated
  {
    m_curveChanged();
  }
  
	void appendControlPoints(unsigned int numPoints) { // migrated
		std::size_t size = m_controlPoints.size();
		
		if (size < 1) {
			return;
		}
		
		// The coordinates of the penultimate point (can be 0,0,0)
		Vector3 penultimate = (size > 1) ? m_controlPoints[size - 2] : Vector3(0,0,0);
		Vector3 ultimate = m_controlPoints[size - 1];
		
		// Calculate the extrapolation vector
		Vector3 extrapolation = ultimate - penultimate;
		
		// greebo: TODO: Use std::vector for this instead of that crappy Array
		ControlPoints newPoints;
		newPoints.resize(size+1);
		
		// Copy the points from the source set
		ControlPoints::iterator target = newPoints.begin();
		for(ControlPoints::iterator source = m_controlPoints.begin(); 
			source != m_controlPoints.end(); 
			++source, ++target)
		{
			*target = *source; 
		}
		
		// The iterator points now to the last element, fill it
		*target = ultimate + extrapolation;
		
		// Replace the old control points with the larger set
		m_controlPoints = newPoints;
		
		// Update the transformation working set
		m_controlPointsTransformed = m_controlPoints;
		
		// Do the maths
		curveChanged();
	}

  void tesselate() // migrated
  {
    if(!m_controlPointsTransformed.empty())
    {
      const std::size_t numSegments = (m_controlPointsTransformed.size() - 1) * 16;
      m_renderCurve.m_vertices.resize(numSegments + 1);
      m_renderCurve.m_vertices[0].vertex = Vertex3f(m_controlPointsTransformed[0]);
      for(std::size_t i = 1; i < numSegments; ++i)
      {
        m_renderCurve.m_vertices[i].vertex = Vertex3f(CatmullRom_evaluate(m_controlPointsTransformed, (1.0 / double(numSegments)) * double(i)));
      }
      m_renderCurve.m_vertices[numSegments].vertex = Vertex3f(m_controlPointsTransformed[m_controlPointsTransformed.size() - 1]);
    }
    else
    {
      m_renderCurve.m_vertices.clear();
    }
  }

  bool parseCurve(const char* value) // migrated
  {
    return ControlPoints_parse(m_controlPoints, value);
  }

  void curveChanged() // migrated
  {
    tesselate();

    m_bounds = AABB();
    for(ControlPoints::iterator i = m_controlPointsTransformed.begin(); i != m_controlPointsTransformed.end(); ++i)
    {
      m_bounds.includePoint(*i);
    }

    m_boundsChanged();
    notify();
  }

  void curveChanged(const std::string& value) // migrated
  {
    if(value.empty() || !parseCurve(value.c_str()))
    {
      m_controlPoints.resize(0);
    }
    m_controlPointsTransformed = m_controlPoints;
    curveChanged();
  }
  typedef MemberCaller1<CatmullRomSpline, const std::string&, &CatmullRomSpline::curveChanged> CurveChangedCaller;
};*/

//const char* const curve_Nurbs = "curve_Nurbs";
//const char* const curve_CatmullRomSpline = "curve_CatmullRomSpline";


#endif
