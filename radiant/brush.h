/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

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

#if !defined(INCLUDED_BRUSH_H)
#define INCLUDED_BRUSH_H

/// \file
/// \brief The brush primitive.
///
/// A collection of planes that define a convex polyhedron.
/// The Boundary-Representation of this primitive is a manifold polygonal mesh.
/// Each face polygon is represented by a list of vertices in a \c Winding.
/// Each vertex is associated with another face that is adjacent to the edge
/// formed by itself and the next vertex in the winding. This information can
/// be used to find edge-pairs and vertex-rings.


#include "debugging/debugging.h"

#include "brush/TexDef.h"
#include "iundo.h"
#include "iselection.h"
#include "irender.h"
#include "imap.h"
#include "ibrush.h"
#include "igl.h"
#include "ifilter.h"
#include "nameable.h"
#include "moduleobserver.h"

#include <set>

#include "cullable.h"
#include "renderable.h"
#include "selectable.h"
#include "editable.h"
#include "mapfile.h"

#include "math/frustum.h"
#include "selectionlib.h"
#include "render.h"
#include "texturelib.h"
#include "container/container.h"
#include "generic/bitfield.h"
#include "signal/signalfwd.h"

#include "winding.h"
#include "brush/TextureProjection.h"

#include "brush/FaceShader.h"
#include "brush/ContentsFlagsValue.h"
#include "brush/FaceTexDef.h"
#include "brush/PlanePoints.h"
#include "brush/FacePlane.h"
#include "brush/Face.h"
#include "brush/SelectableComponents.h"
#include "brush/BrushClass.h"

enum EBrushType
{
  eBrushTypeQuake,
  eBrushTypeQuake2,
  eBrushTypeQuake3,
  eBrushTypeQuake3BP,
  eBrushTypeDoom3,
  eBrushTypeQuake4,
  eBrushTypeHalfLife,
};


#define BRUSH_CONNECTIVITY_DEBUG 0
#define BRUSH_DEGENERATE_DEBUG 0

template<typename TextOuputStreamType>
inline TextOuputStreamType& ostream_write(TextOuputStreamType& ostream, const Matrix4& m)
{
  return ostream << "(" << m[0] << " " << m[1] << " " << m[2] << " " << m[3] << ", "
    << m[4] << " " << m[5] << " " << m[6] << " " << m[7] << ", "
    << m[8] << " " << m[9] << " " << m[10] << " " << m[11] << ", "
    << m[12] << " " << m[13] << " " << m[14] << " " << m[15] << ")";
}

inline void Winding_DrawWireframe(const Winding& winding)
{
  glVertexPointer(3, GL_FLOAT, sizeof(WindingVertex), &winding.points.data()->vertex);
  glDrawArrays(GL_LINE_LOOP, 0, GLsizei(winding.numpoints));
}



#include "shaderlib.h"


inline bool check_plane_is_integer(const PlanePoints& planePoints)
{
  return !float_is_integer(planePoints[0][0])
    || !float_is_integer(planePoints[0][1])
    || !float_is_integer(planePoints[0][2])
    || !float_is_integer(planePoints[1][0])
    || !float_is_integer(planePoints[1][1])
    || !float_is_integer(planePoints[1][2])
    || !float_is_integer(planePoints[2][0])
    || !float_is_integer(planePoints[2][1])
    || !float_is_integer(planePoints[2][2]);
}


inline void planepts_print(const PlanePoints& planePoints, TextOutputStream& ostream)
{
  ostream << "( " << planePoints[0][0] << " " << planePoints[0][1] << " " << planePoints[0][2] << " ) "
    << "( " << planePoints[1][0] << " " << planePoints[1][1] << " " << planePoints[1][2] << " ) "
    << "( " << planePoints[2][0] << " " << planePoints[2][1] << " " << planePoints[2][2] << " )";
}

/*inline double quantiseInteger(double f)
{
  return float_to_integer(f);
}*/

void Brush_addTextureChangedCallback(const SignalHandler& callback);
void Brush_textureChanged();


extern bool g_brush_texturelock_enabled;






class Brush;
typedef std::vector<Brush*> brush_vector_t;

class FaceInstance;

class FaceInstanceSet
{
  typedef SelectionList<FaceInstance> FaceInstances;
  FaceInstances m_faceInstances;
public:
  void insert(FaceInstance& faceInstance)
  {
    m_faceInstances.append(faceInstance);
  }
  void erase(FaceInstance& faceInstance)
  {
    m_faceInstances.erase(faceInstance);
  }

  template<typename Functor>
  void foreach(Functor functor)
  {
    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      functor(*(*i));
    }
  }

  bool empty() const
  {
    return m_faceInstances.empty();
  }
  FaceInstance& last() const
  {
    return m_faceInstances.back();
  }
};

extern FaceInstanceSet g_SelectedFaceInstances;

typedef std::list<std::size_t> VertexSelection;

inline VertexSelection::iterator VertexSelection_find(VertexSelection& self, std::size_t value)
{
  return std::find(self.begin(), self.end(), value);
}

inline VertexSelection::const_iterator VertexSelection_find(const VertexSelection& self, std::size_t value)
{
  return std::find(self.begin(), self.end(), value);
}

inline VertexSelection::iterator VertexSelection_insert(VertexSelection& self, std::size_t value)
{
  VertexSelection::iterator i = VertexSelection_find(self, value);
  if(i == self.end())
  {
    self.push_back(value);
    return --self.end();
  }
  return i;
}
inline void VertexSelection_erase(VertexSelection& self, std::size_t value)
{
  VertexSelection::iterator i = VertexSelection_find(self, value);
  if(i != self.end())
  {
    self.erase(i);
  }
}

inline bool triangle_reversed(std::size_t x, std::size_t y, std::size_t z)
{
  return !((x < y && y < z) || (z < x && x < y) || (y < z && z < x));
}
template<typename Element>
inline Vector3 triangle_cross(const BasicVector3<Element>& x, const BasicVector3<Element> y, const BasicVector3<Element>& z)
{
  return (y - x).crossProduct(z - x);
}
template<typename Element>
inline bool triangles_same_winding(const BasicVector3<Element>& x1, const BasicVector3<Element> y1, const BasicVector3<Element>& z1, const BasicVector3<Element>& x2, const BasicVector3<Element> y2, const BasicVector3<Element>& z2)
{
  return triangle_cross(x1, y1, z1).dot(triangle_cross(x2, y2, z2)) > 0;
}


typedef const Plane3* PlanePointer;
typedef PlanePointer* PlanesIterator;

class VectorLightList : public LightList
{
  typedef std::vector<const RendererLight*> Lights;
  Lights m_lights;
public:
  void addLight(const RendererLight& light)
  {
    m_lights.push_back(&light);
  }
  void clear()
  {
    m_lights.clear();
  }
  void evaluateLights() const
  {
  }
  void lightsChanged() const
  {
  }
  void forEachLight(const RendererLightCallback& callback) const
  {
    for(Lights::const_iterator i = m_lights.begin(); i != m_lights.end(); ++i)
    {
      callback(*(*i));
    }
  }
};

class FaceInstance
{
  Face* m_face;
  ObservedSelectable m_selectable;
  ObservedSelectable m_selectableVertices;
  ObservedSelectable m_selectableEdges;
  SelectionChangeCallback m_selectionChanged;

  VertexSelection m_vertexSelection;
  VertexSelection m_edgeSelection;

public:
  mutable VectorLightList m_lights;

  FaceInstance(Face& face, const SelectionChangeCallback& observer) :
    m_face(&face),
    m_selectable(SelectedChangedCaller(*this)),
    m_selectableVertices(observer),
    m_selectableEdges(observer),
    m_selectionChanged(observer)
  {
  }
  FaceInstance(const FaceInstance& other) :
    m_face(other.m_face),
    m_selectable(SelectedChangedCaller(*this)),
    m_selectableVertices(other.m_selectableVertices),
    m_selectableEdges(other.m_selectableEdges),
    m_selectionChanged(other.m_selectionChanged)
  {
  }
  FaceInstance& operator=(const FaceInstance& other)
  {
    m_face = other.m_face;
    return *this;
  }

  Face& getFace()
  {
    return *m_face;
  }
  const Face& getFace() const
  {
    return *m_face;
  }

  void selectedChanged(const Selectable& selectable)
  {
    if(selectable.isSelected())
    {
      g_SelectedFaceInstances.insert(*this);
    }
    else
    {
      g_SelectedFaceInstances.erase(*this);
    }
    m_selectionChanged(selectable);
  }
  typedef MemberCaller1<FaceInstance, const Selectable&, &FaceInstance::selectedChanged> SelectedChangedCaller;

  bool selectedVertices() const
  {
    return !m_vertexSelection.empty();
  }
  bool selectedEdges() const
  {
    return !m_edgeSelection.empty();
  }
  bool isSelected() const
  {
    return m_selectable.isSelected();
  }

  bool selectedComponents() const
  {
    return selectedVertices() || selectedEdges() || isSelected();
  }
  bool selectedComponents(SelectionSystem::EComponentMode mode) const
  {
    switch(mode)
    {
    case SelectionSystem::eVertex:
      return selectedVertices();
    case SelectionSystem::eEdge:
      return selectedEdges();
    case SelectionSystem::eFace:
      return isSelected();
    default:
      return false;
    }
  }
  void setSelected(SelectionSystem::EComponentMode mode, bool select)
  {
    switch(mode)
    {
    case SelectionSystem::eFace:
      m_selectable.setSelected(select);
      break;
    case SelectionSystem::eVertex:
      ASSERT_MESSAGE(!select, "select-all not supported");

      m_vertexSelection.clear();
      m_selectableVertices.setSelected(false);
      break;
    case SelectionSystem::eEdge:
      ASSERT_MESSAGE(!select, "select-all not supported");

      m_edgeSelection.clear();
      m_selectableEdges.setSelected(false);
      break;
    default:
      break;
    }
  }

  template<typename Functor>
  void SelectedVertices_foreach(Functor functor) const
  {
    for(VertexSelection::const_iterator i = m_vertexSelection.begin(); i != m_vertexSelection.end(); ++i)
    {
      std::size_t index = Winding_FindAdjacent(getFace().getWinding(), *i);
      if(index != c_brush_maxFaces)
      {
        functor(getFace().getWinding()[index].vertex);
      }
    }
  }
  template<typename Functor>
  void SelectedEdges_foreach(Functor functor) const
  {
    for(VertexSelection::const_iterator i = m_edgeSelection.begin(); i != m_edgeSelection.end(); ++i)
    {
      std::size_t index = Winding_FindAdjacent(getFace().getWinding(), *i);
      if(index != c_brush_maxFaces)
      {
        const Winding& winding = getFace().getWinding();
        std::size_t adjacent = Winding_next(winding, index);
        functor(vector3_mid(winding[index].vertex, winding[adjacent].vertex));
      }
    }
  }
  template<typename Functor>
  void SelectedFaces_foreach(Functor functor) const
  {
    if(isSelected())
    {
      functor(centroid());
    }
  }

  template<typename Functor>
  void SelectedComponents_foreach(Functor functor) const
  {
    SelectedVertices_foreach(functor);
    SelectedEdges_foreach(functor);
    SelectedFaces_foreach(functor);
  }

  void iterate_selected(AABB& aabb) const
  {
    SelectedComponents_foreach(AABBExtendByPoint(aabb));
  }

  class RenderablePointVectorPushBack
  {
    RenderablePointVector& m_points;
  public:
    RenderablePointVectorPushBack(RenderablePointVector& points) : m_points(points)
    {
    }
    void operator()(const Vector3& point) const
    {
      const Colour4b colour_selected(0, 0, 255, 255);
      m_points.push_back(PointVertex(point, colour_selected));
    }
  };
  
  void iterate_selected(RenderablePointVector& points) const
  {
    SelectedComponents_foreach(RenderablePointVectorPushBack(points));
  }
  
  bool intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    return m_face->intersectVolume(volume, localToWorld);
  }

  void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    if(m_face->contributes() && intersectVolume(volume, localToWorld))
    {
      renderer.PushState();
      if(selectedComponents())
      {
        renderer.Highlight(Renderer::eFace);
      }
      m_face->render(renderer, localToWorld);
      renderer.PopState();
    }
  }

  void testSelect(SelectionTest& test, SelectionIntersection& best)
  {
      m_face->testSelect(test, best);
  }

  void testSelect(Selector& selector, SelectionTest& test)
  {
    SelectionIntersection best;
    testSelect(test, best);
    if(best.valid())
    {
      Selector_add(selector, m_selectable, best);
    }
  }
  void testSelect_centroid(Selector& selector, SelectionTest& test)
  {
    if(m_face->contributes())
    {
      SelectionIntersection best;
      m_face->testSelect_centroid(test, best);
      if(best.valid())
      {
        Selector_add(selector, m_selectable, best);
      }
    }
  }

  void selectPlane(Selector& selector, const Line& line, PlanesIterator first, PlanesIterator last, const PlaneCallback& selectedPlaneCallback)
  {
    for(Winding::const_iterator i = getFace().getWinding().begin(); i != getFace().getWinding().end(); ++i)
    {
      Vector3 v(line_closest_point(line, (*i).vertex) - (*i).vertex);
      double dot = getFace().plane3().normal().dot(v);
      if(dot <= 0)
      {
        return;
      }
    }

    Selector_add(selector, m_selectable);

    selectedPlaneCallback(getFace().plane3());
  }
  void selectReversedPlane(Selector& selector, const SelectedPlanes& selectedPlanes)
  {
    if(selectedPlanes.contains(-(getFace().plane3())))
    {
      Selector_add(selector, m_selectable);
    }
  }

  void transformComponents(const Matrix4& matrix)
  {
    if(isSelected())
    {
      m_face->transform(matrix, false);
    }
    if(selectedVertices())
    {
      if(m_vertexSelection.size() == 1)
      {
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
        m_face->assign_planepts(m_face->m_move_planeptsTransformed);
      }
      else if(m_vertexSelection.size() == 2)
      {
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[2]);
        m_face->assign_planepts(m_face->m_move_planeptsTransformed);
      }
      else if(m_vertexSelection.size() >= 3)
      {
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[0]);
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[2]);
        m_face->assign_planepts(m_face->m_move_planeptsTransformed);
      }
    }
    if(selectedEdges())
    {
      if(m_edgeSelection.size() == 1)
      {
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[0]);
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
        m_face->assign_planepts(m_face->m_move_planeptsTransformed);
      }
      else if(m_edgeSelection.size() >= 2)
      {
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[0]);
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[1]);
        matrix4_transform_point(matrix, m_face->m_move_planeptsTransformed[2]);
        m_face->assign_planepts(m_face->m_move_planeptsTransformed);
      }
    }
  }

  void snapto(float snap)
  {
    m_face->snapto(snap);
  }

  void snapComponents(float snap)
  {
    if(isSelected())
    {
      snapto(snap);
    }
    if(selectedVertices())
    {
      vector3_snap(m_face->m_move_planepts[0], snap);
      vector3_snap(m_face->m_move_planepts[1], snap);
      vector3_snap(m_face->m_move_planepts[2], snap);
      m_face->assign_planepts(m_face->m_move_planepts);
      planepts_assign(m_face->m_move_planeptsTransformed, m_face->m_move_planepts);
      m_face->freezeTransform();
    }
    if(selectedEdges())
    {
      vector3_snap(m_face->m_move_planepts[0], snap);
      vector3_snap(m_face->m_move_planepts[1], snap);
      vector3_snap(m_face->m_move_planepts[2], snap);
      m_face->assign_planepts(m_face->m_move_planepts);
      planepts_assign(m_face->m_move_planeptsTransformed, m_face->m_move_planepts);
      m_face->freezeTransform();
    }
  }
  void update_move_planepts_vertex(std::size_t index)
  {
    m_face->update_move_planepts_vertex(index, m_face->m_move_planepts);
  }
  void update_move_planepts_vertex2(std::size_t index, std::size_t other)
  {
    const std::size_t numpoints = m_face->getWinding().numpoints;
    ASSERT_MESSAGE(index < numpoints, "select_vertex: invalid index");

    const std::size_t opposite = Winding_Opposite(m_face->getWinding(), index, other);

    if(triangle_reversed(index, other, opposite))
    {
      std::swap(index, other);
    }

    ASSERT_MESSAGE(
      triangles_same_winding(
        m_face->getWinding()[opposite].vertex,
        m_face->getWinding()[index].vertex,
        m_face->getWinding()[other].vertex,
        m_face->getWinding()[0].vertex,
        m_face->getWinding()[1].vertex,
        m_face->getWinding()[2].vertex
      ),
      "update_move_planepts_vertex2: error"
    )

    m_face->m_move_planepts[0] = m_face->getWinding()[opposite].vertex;
    m_face->m_move_planepts[1] = m_face->getWinding()[index].vertex;
    m_face->m_move_planepts[2] = m_face->getWinding()[other].vertex;
    planepts_quantise(m_face->m_move_planepts, GRID_MIN); // winding points are very inaccurate
  }
  void update_selection_vertex()
  {
    if(m_vertexSelection.size() == 0)
    {
      m_selectableVertices.setSelected(false);
    }
    else
    {
      m_selectableVertices.setSelected(true);

      if(m_vertexSelection.size() == 1)
      {
        std::size_t index = Winding_FindAdjacent(getFace().getWinding(), *m_vertexSelection.begin());

        if(index != c_brush_maxFaces)
        {
          update_move_planepts_vertex(index);
        }
      }
      else if(m_vertexSelection.size() == 2)
      {
        std::size_t index = Winding_FindAdjacent(getFace().getWinding(), *m_vertexSelection.begin());
        std::size_t other = Winding_FindAdjacent(getFace().getWinding(), *(++m_vertexSelection.begin()));

        if(index != c_brush_maxFaces
          && other != c_brush_maxFaces)
        {
          update_move_planepts_vertex2(index, other);
        }
      }
    }
  }
  void select_vertex(std::size_t index, bool select)
  {
    if(select)
    {
      VertexSelection_insert(m_vertexSelection, getFace().getWinding()[index].adjacent);
    }
    else
    {
      VertexSelection_erase(m_vertexSelection, getFace().getWinding()[index].adjacent);
    }

    SceneChangeNotify();
    update_selection_vertex();
  }

  bool selected_vertex(std::size_t index) const
  {
    return VertexSelection_find(m_vertexSelection, getFace().getWinding()[index].adjacent) != m_vertexSelection.end();
  }

  void update_move_planepts_edge(std::size_t index)
  {
    std::size_t numpoints = m_face->getWinding().numpoints;
    ASSERT_MESSAGE(index < numpoints, "select_edge: invalid index");

    std::size_t adjacent = Winding_next(m_face->getWinding(), index);
    std::size_t opposite = Winding_Opposite(m_face->getWinding(), index);
    m_face->m_move_planepts[0] = m_face->getWinding()[index].vertex;
    m_face->m_move_planepts[1] = m_face->getWinding()[adjacent].vertex;
    m_face->m_move_planepts[2] = m_face->getWinding()[opposite].vertex;
    planepts_quantise(m_face->m_move_planepts, GRID_MIN); // winding points are very inaccurate
  }
  void update_selection_edge()
  {
    if(m_edgeSelection.size() == 0)
    {
      m_selectableEdges.setSelected(false);
    }
    else
    {
      m_selectableEdges.setSelected(true);

      if(m_edgeSelection.size() == 1)
      {
        std::size_t index = Winding_FindAdjacent(getFace().getWinding(), *m_edgeSelection.begin());

        if(index != c_brush_maxFaces)
        {
          update_move_planepts_edge(index);
        }
      }
    }
  }
  void select_edge(std::size_t index, bool select)
  {
    if(select)
    {
      VertexSelection_insert(m_edgeSelection, getFace().getWinding()[index].adjacent);
    }
    else
    {
      VertexSelection_erase(m_edgeSelection, getFace().getWinding()[index].adjacent);
    }

    SceneChangeNotify();
    update_selection_edge();
  }

  bool selected_edge(std::size_t index) const
  {
    return VertexSelection_find(m_edgeSelection, getFace().getWinding()[index].adjacent) != m_edgeSelection.end();
  }

  const Vector3& centroid() const
  {
    return m_face->centroid();
  }

  void connectivityChanged()
  {
    // This occurs when a face is added or removed.
    // The current vertex and edge selections no longer valid and must be cleared.
    m_vertexSelection.clear();
    m_selectableVertices.setSelected(false);
    m_edgeSelection.clear();
    m_selectableEdges.setSelected(false);
  }
};

class BrushClipPlane : public OpenGLRenderable
{
  Plane3 m_plane;
  Winding m_winding;
  static Shader* m_state;
public:
  static void constructStatic()
  {
    m_state = GlobalShaderCache().capture("$CLIPPER_OVERLAY");
  }
  static void destroyStatic()
  {
    GlobalShaderCache().release("$CLIPPER_OVERLAY");
  }

  void setPlane(const Brush& brush, const Plane3& plane)
  {
    m_plane = plane;
    if(m_plane.isValid())
    {
      brush.windingForClipPlane(m_winding, m_plane);
    }
    else
    {
      m_winding.resize(0);
    }
  }

  void render(RenderStateFlags state) const
  {
    if((state & RENDER_FILL) != 0)
    {
      Winding_Draw(m_winding, m_plane.normal(), state);
    }
    else
    {
      Winding_DrawWireframe(m_winding);
    }
  }

  void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    renderer.SetState(m_state, Renderer::eWireframeOnly);
    renderer.SetState(m_state, Renderer::eFullMaterials);
    renderer.addRenderable(*this, localToWorld);
  }
};

inline void Face_addLight(const FaceInstance& face, const Matrix4& localToWorld, const RendererLight& light)
{
  const Plane3& facePlane = face.getFace().plane3();
  const Vector3& origin = light.aabb().origin;
  Plane3 tmp(localToWorld.transform(Plane3(facePlane.normal(), -facePlane.dist())));
  if(!plane3_test_point(tmp, origin)
    || !plane3_test_point(tmp, origin + light.offset()))
  {
    face.m_lights.addLight(light);
  }
}



typedef std::vector<FaceInstance> FaceInstances;

class EdgeInstance : public Selectable
{
  FaceInstances& m_faceInstances;
  SelectableEdge* m_edge;

  void select_edge(bool select)
  {
    FaceVertexId faceVertex = m_edge->m_faceVertex;
    m_faceInstances[faceVertex.getFace()].select_edge(faceVertex.getVertex(), select);
    faceVertex = next_edge(m_edge->m_faces, faceVertex);
    m_faceInstances[faceVertex.getFace()].select_edge(faceVertex.getVertex(), select);
  }
  bool selected_edge() const
  {
    FaceVertexId faceVertex = m_edge->m_faceVertex;
    if(!m_faceInstances[faceVertex.getFace()].selected_edge(faceVertex.getVertex()))
    {
      return false;
    }
    faceVertex = next_edge(m_edge->m_faces, faceVertex);
    if(!m_faceInstances[faceVertex.getFace()].selected_edge(faceVertex.getVertex()))
    {
      return false;
    }

    return true;
  }

public:
  EdgeInstance(FaceInstances& faceInstances, SelectableEdge& edge)
    : m_faceInstances(faceInstances), m_edge(&edge)
  {
  }
  EdgeInstance& operator=(const EdgeInstance& other)
  {
    m_edge = other.m_edge;
    return *this;
  }

  void setSelected(bool select)
  {
    select_edge(select);
  }
  bool isSelected() const
  {
    return selected_edge();
  }


  void testSelect(Selector& selector, SelectionTest& test)
  {
    SelectionIntersection best;
    m_edge->testSelect(test, best);
    if(best.valid())
    {
      Selector_add(selector, *this, best);
    }
  }
};

class VertexInstance : public Selectable
{
  FaceInstances& m_faceInstances;
  SelectableVertex* m_vertex;

  void select_vertex(bool select)
  {
    FaceVertexId faceVertex = m_vertex->m_faceVertex;
    do
    {
      m_faceInstances[faceVertex.getFace()].select_vertex(faceVertex.getVertex(), select);
      faceVertex = next_vertex(m_vertex->m_faces, faceVertex);
    }
    while(faceVertex.getFace() != m_vertex->m_faceVertex.getFace());
  }
  bool selected_vertex() const
  {
    FaceVertexId faceVertex = m_vertex->m_faceVertex;
    do
    {
      if(!m_faceInstances[faceVertex.getFace()].selected_vertex(faceVertex.getVertex()))
      {
        return false;
      }
      faceVertex = next_vertex(m_vertex->m_faces, faceVertex);
    }
    while(faceVertex.getFace() != m_vertex->m_faceVertex.getFace());
    return true;
  }

public:
  VertexInstance(FaceInstances& faceInstances, SelectableVertex& vertex)
    : m_faceInstances(faceInstances), m_vertex(&vertex)
  {
  }
  VertexInstance& operator=(const VertexInstance& other)
  {
    m_vertex = other.m_vertex;
    return *this;
  }

  void setSelected(bool select)
  {
    select_vertex(select);
  }
  bool isSelected() const
  {
    return selected_vertex();
  }

  void testSelect(Selector& selector, SelectionTest& test)
  {
    SelectionIntersection best;
    m_vertex->testSelect(test, best);
    if(best.valid())
    {
      Selector_add(selector, *this, best);
    }
  }
};

class BrushInstanceVisitor
{
public:
  virtual void visit(FaceInstance& face) const = 0;
};

class BrushInstance :
public BrushObserver,
public scene::Instance,
public Selectable,
public Renderable,
public SelectionTestable,
public ComponentSelectionTestable,
public ComponentEditable,
public ComponentSnappable,
public PlaneSelectable,
public LightCullable
{
  class TypeCasts
  {
    InstanceTypeCastTable m_casts;
  public:
    TypeCasts()
    {
      InstanceStaticCast<BrushInstance, Selectable>::install(m_casts);
      InstanceContainedCast<BrushInstance, Bounded>::install(m_casts);
      InstanceContainedCast<BrushInstance, Cullable>::install(m_casts);
      InstanceStaticCast<BrushInstance, Renderable>::install(m_casts);
      InstanceStaticCast<BrushInstance, SelectionTestable>::install(m_casts);
      InstanceStaticCast<BrushInstance, ComponentSelectionTestable>::install(m_casts);
      InstanceStaticCast<BrushInstance, ComponentEditable>::install(m_casts);
      InstanceStaticCast<BrushInstance, ComponentSnappable>::install(m_casts);
      InstanceStaticCast<BrushInstance, PlaneSelectable>::install(m_casts);
      InstanceIdentityCast<BrushInstance>::install(m_casts);
      InstanceContainedCast<BrushInstance, Transformable>::install(m_casts);
    }
    InstanceTypeCastTable& get()
    {
      return m_casts;
    }
  };


  Brush& m_brush;

  FaceInstances m_faceInstances;

  typedef std::vector<EdgeInstance> EdgeInstances;
  EdgeInstances m_edgeInstances;
  typedef std::vector<VertexInstance> VertexInstances;
  VertexInstances m_vertexInstances;

  ObservedSelectable m_selectable;

  mutable RenderableWireframe m_render_wireframe;
  mutable RenderablePointVector m_render_selected;
  mutable AABB m_aabb_component;
  mutable Array<PointVertex> m_faceCentroidPointsCulled;
  RenderablePointArray m_render_faces_wireframe;
  mutable bool m_viewChanged; // requires re-evaluation of view-dependent cached data

  BrushClipPlane m_clipPlane;

  static Shader* m_state_selpoint;

  const LightList* m_lightList;

  TransformModifier m_transform;

  BrushInstance(const BrushInstance& other); // NOT COPYABLE
  BrushInstance& operator=(const BrushInstance& other); // NOT ASSIGNABLE
public:
  static Counter* m_counter;

  typedef LazyStatic<TypeCasts> StaticTypeCasts;

  void lightsChanged()
  {
    m_lightList->lightsChanged();
  }
  typedef MemberCaller<BrushInstance, &BrushInstance::lightsChanged> LightsChangedCaller;

  STRING_CONSTANT(Name, "BrushInstance");

  BrushInstance(const scene::Path& path, scene::Instance* parent, Brush& brush) :
    Instance(path, parent, this, StaticTypeCasts::instance().get()),
    m_brush(brush),
    m_selectable(SelectedChangedCaller(*this)),
    m_render_selected(GL_POINTS),
    m_render_faces_wireframe(m_faceCentroidPointsCulled, GL_POINTS),
    m_viewChanged(false),
    m_transform(Brush::TransformChangedCaller(m_brush), ApplyTransformCaller(*this))
  {
    m_brush.instanceAttach(Instance::path());
    m_brush.attach(*this);
    m_counter->increment();

    m_lightList = &GlobalShaderCache().attach(*this);
    m_brush.m_lightsChanged = LightsChangedCaller(*this); ///\todo Make this work with instancing.

    Instance::setTransformChangedCallback(LightsChangedCaller(*this));
  }
  ~BrushInstance()
  {
    Instance::setTransformChangedCallback(Callback());

    m_brush.m_lightsChanged = Callback();
    GlobalShaderCache().detach(*this);

    m_counter->decrement();
    m_brush.detach(*this);
    m_brush.instanceDetach(Instance::path());
  }

  Brush& getBrush()
  {
    return m_brush;
  }
  const Brush& getBrush() const
  {
    return m_brush;
  }

  Bounded& get(NullType<Bounded>)
  {
    return m_brush;
  }
  Cullable& get(NullType<Cullable>)
  {
    return m_brush;
  }
  Transformable& get(NullType<Transformable>)
  {
    return m_transform;
  }

  void selectedChanged(const Selectable& selectable)
  {
    GlobalSelectionSystem().getObserver(SelectionSystem::ePrimitive)(selectable);
    GlobalSelectionSystem().onSelectedChanged(*this, selectable);

    Instance::selectedChanged();
  }
  typedef MemberCaller1<BrushInstance, const Selectable&, &BrushInstance::selectedChanged> SelectedChangedCaller;

  void selectedChangedComponent(const Selectable& selectable)
  {
    GlobalSelectionSystem().getObserver(SelectionSystem::eComponent)(selectable);
    GlobalSelectionSystem().onComponentSelection(*this, selectable);
  }
  typedef MemberCaller1<BrushInstance, const Selectable&, &BrushInstance::selectedChangedComponent> SelectedChangedComponentCaller;

  const BrushInstanceVisitor& forEachFaceInstance(const BrushInstanceVisitor& visitor)
  {
    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      visitor.visit(*i);
    }
    return visitor;
  }

  static void constructStatic()
  {
    m_state_selpoint = GlobalShaderCache().capture("$SELPOINT");
  }
  static void destroyStatic()
  {
    GlobalShaderCache().release("$SELPOINT");
  }

  void clear()
  {
    m_faceInstances.clear();
  }
  void reserve(std::size_t size)
  {
    m_faceInstances.reserve(size);
  }

  void push_back(Face& face)
  {
    m_faceInstances.push_back(FaceInstance(face, SelectedChangedComponentCaller(*this)));
  }
  void pop_back()
  {
    ASSERT_MESSAGE(!m_faceInstances.empty(), "erasing invalid element");
    m_faceInstances.pop_back();
  }
  void erase(std::size_t index)
  {
    ASSERT_MESSAGE(index < m_faceInstances.size(), "erasing invalid element");
    m_faceInstances.erase(m_faceInstances.begin() + index);
  }
  void connectivityChanged()
  {
    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).connectivityChanged();
    }
  }

  void edge_clear()
  {
    m_edgeInstances.clear();
  }
  void edge_push_back(SelectableEdge& edge)
  {
    m_edgeInstances.push_back(EdgeInstance(m_faceInstances, edge));
  }

  void vertex_clear()
  {
    m_vertexInstances.clear();
  }
  void vertex_push_back(SelectableVertex& vertex)
  {
    m_vertexInstances.push_back(VertexInstance(m_faceInstances, vertex));
  }

  void DEBUG_verify() const
  {
    ASSERT_MESSAGE(m_faceInstances.size() == m_brush.DEBUG_size(), "FATAL: mismatch");
  }

  bool isSelected() const
  {
    return m_selectable.isSelected();
  }
  void setSelected(bool select)
  {
    m_selectable.setSelected(select);
  }

  void update_selected() const
  {
    m_render_selected.clear();
    for(FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      if((*i).getFace().contributes())
      {
        (*i).iterate_selected(m_render_selected);
      }
    }
  }

  void evaluateViewDependent(const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    if(m_viewChanged)
    {
      m_viewChanged = false;

      bool faces_visible[c_brush_maxFaces];
      {
        bool* j = faces_visible;
        for(FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i, ++j)
        {
        	// Check if face is filtered before adding to visibility matrix
        	if (GlobalFilterSystem().isVisible("texture", i->getFace().GetShader()))
				*j = i->intersectVolume(volume, localToWorld);
			else
				*j = false;
        }
      }

      m_brush.update_wireframe(m_render_wireframe, faces_visible);
      m_brush.update_faces_wireframe(m_faceCentroidPointsCulled, faces_visible);
    }
  }

  void renderComponentsSelected(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    m_brush.evaluateBRep();

    update_selected();
    if(!m_render_selected.empty())
    {
      renderer.Highlight(Renderer::ePrimitive, false);
      renderer.SetState(m_state_selpoint, Renderer::eWireframeOnly);
      renderer.SetState(m_state_selpoint, Renderer::eFullMaterials);
      renderer.addRenderable(m_render_selected, localToWorld);
    }
  }

  void renderComponents(Renderer& renderer, const VolumeTest& volume) const
  {
    m_brush.evaluateBRep();

    const Matrix4& localToWorld = Instance::localToWorld();

    renderer.SetState(m_brush.m_state_point, Renderer::eWireframeOnly);
    renderer.SetState(m_brush.m_state_point, Renderer::eFullMaterials);

    if(volume.fill() && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eFace)
    {
      evaluateViewDependent(volume, localToWorld);
      renderer.addRenderable(m_render_faces_wireframe, localToWorld);
    }
    else
    {
      m_brush.renderComponents(GlobalSelectionSystem().ComponentMode(), renderer, volume, localToWorld);
    }
  }

  void renderClipPlane(Renderer& renderer, const VolumeTest& volume) const
  {
    if(GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eClip && isSelected())
    {
      m_clipPlane.render(renderer, volume, localToWorld());
    }
  }

  void renderCommon(Renderer& renderer, const VolumeTest& volume) const
  {
    bool componentMode = GlobalSelectionSystem().Mode() == SelectionSystem::eComponent;
    
    if(componentMode && isSelected())
    {
      renderComponents(renderer, volume);
    }
    
    if(parentSelected())
    {
      if(!componentMode)
      {
        renderer.Highlight(Renderer::eFace);
      }
      renderer.Highlight(Renderer::ePrimitive);
    }
  }

  void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    //renderCommon(renderer, volume);

    m_lightList->evaluateLights();

    for(FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      renderer.setLights((*i).m_lights);
      (*i).render(renderer, volume, localToWorld);
    }

    renderComponentsSelected(renderer, volume, localToWorld);
  }

  void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    //renderCommon(renderer, volume);

    evaluateViewDependent(volume, localToWorld);

    if(m_render_wireframe.m_size != 0)
    {
      renderer.addRenderable(m_render_wireframe, localToWorld);
    }

    renderComponentsSelected(renderer, volume, localToWorld);
  }

  void renderSolid(Renderer& renderer, const VolumeTest& volume) const
  {
    m_brush.evaluateBRep();

    renderClipPlane(renderer, volume);

    renderSolid(renderer, volume, localToWorld());
  }

  void renderWireframe(Renderer& renderer, const VolumeTest& volume) const
  {
    m_brush.evaluateBRep();

    renderClipPlane(renderer, volume);

    renderWireframe(renderer, volume, localToWorld());
  }

  void viewChanged() const
  {
    m_viewChanged = true;
  }

  void testSelect(Selector& selector, SelectionTest& test)
  {
    test.BeginMesh(localToWorld());

    SelectionIntersection best;
    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).testSelect(test, best);
    }
    if(best.valid())
    {
      selector.addIntersection(best);
    }
  }

  bool isSelectedComponents() const
  {
    for(FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      if((*i).selectedComponents())
      {
        return true;
      }
    }
    return false;
  }
  void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode)
  {
    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).setSelected(mode, select);
    }
  }
  void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode)
  {
    test.BeginMesh(localToWorld());

    switch(mode)
    {
    case SelectionSystem::eVertex:
      {
        for(VertexInstances::iterator i = m_vertexInstances.begin(); i != m_vertexInstances.end(); ++i)
        {
          (*i).testSelect(selector, test);
        }
      }
      break;
    case SelectionSystem::eEdge:
      {
        for(EdgeInstances::iterator i = m_edgeInstances.begin(); i != m_edgeInstances.end(); ++i)
        {
          (*i).testSelect(selector, test);
        }
      }
      break;
    case SelectionSystem::eFace:
      {
        if(test.getVolume().fill())
        {
          for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
          {
            (*i).testSelect(selector, test);
          }
        }
        else
        {
          for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
          {
            (*i).testSelect_centroid(selector, test);
          }
        }
      }
      break;
    default:
      break;
    }
  }

  void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback)
  {
    test.BeginMesh(localToWorld());

    PlanePointer brushPlanes[c_brush_maxFaces];
    PlanesIterator j = brushPlanes;

    for(Brush::const_iterator i = m_brush.begin(); i != m_brush.end(); ++i)
    {
      *j++ = &(*i)->plane3();
    }

    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).selectPlane(selector, Line(test.getNear(), test.getFar()), brushPlanes, j, selectedPlaneCallback);
    }
  }
  void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
  {
    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).selectReversedPlane(selector, selectedPlanes);
    }
  }


  void transformComponents(const Matrix4& matrix)
  {
    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).transformComponents(matrix);
    }
  }
  const AABB& getSelectedComponentsBounds() const
  {
    m_aabb_component = AABB();

    for(FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).iterate_selected(m_aabb_component);
    }

    return m_aabb_component;
  }

  void snapComponents(float snap)
  {
    for(FaceInstances::iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).snapComponents(snap);
    }
  }
  void evaluateTransform()
  {
    Matrix4 matrix(m_transform.calculateTransform());
    //globalOutputStream() << "matrix: " << matrix << "\n";

    if(m_transform.getType() == TRANSFORM_PRIMITIVE)
    {
      m_brush.transform(matrix);
    }
    else
    {
      transformComponents(matrix);
    }
  }
  void applyTransform()
  {
    m_brush.revertTransform();
    evaluateTransform();
    m_brush.freezeTransform();
  }
  typedef MemberCaller<BrushInstance, &BrushInstance::applyTransform> ApplyTransformCaller;

  void setClipPlane(const Plane3& plane)
  {
    m_clipPlane.setPlane(m_brush, plane);
  }

  bool testLight(const RendererLight& light) const
  {
    return light.testAABB(worldAABB());
  }
  void insertLight(const RendererLight& light)
  {
    const Matrix4& localToWorld = Instance::localToWorld();
    for(FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      Face_addLight(*i, localToWorld, light);
    }
  }
  void clearLights()
  {
    for(FaceInstances::const_iterator i = m_faceInstances.begin(); i != m_faceInstances.end(); ++i)
    {
      (*i).m_lights.clear();
    }
  }
};

inline BrushInstance* Instance_getBrush(scene::Instance& instance)
{
  return InstanceTypeCast<BrushInstance>::cast(instance);
}


template<typename Functor>
class BrushSelectedVisitor : public SelectionSystem::Visitor
{
  const Functor& m_functor;
public:
  BrushSelectedVisitor(const Functor& functor) : m_functor(functor)
  {
  }
  void visit(scene::Instance& instance) const
  {
    BrushInstance* brush = Instance_getBrush(instance);
    if(brush != 0)
    {
      m_functor(*brush);
    }
  }
};

template<typename Functor>
inline const Functor& Scene_forEachSelectedBrush(const Functor& functor)
{
  GlobalSelectionSystem().foreachSelected(BrushSelectedVisitor<Functor>(functor));
  return functor;
}

template<typename Functor>
class BrushVisibleSelectedVisitor : public SelectionSystem::Visitor
{
  const Functor& m_functor;
public:
  BrushVisibleSelectedVisitor(const Functor& functor) : m_functor(functor)
  {
  }
  void visit(scene::Instance& instance) const
  {
    BrushInstance* brush = Instance_getBrush(instance);
    if(brush != 0
      && instance.path().top().get().visible())
    {
      m_functor(*brush);
    }
  }
};

template<typename Functor>
inline const Functor& Scene_forEachVisibleSelectedBrush(const Functor& functor)
{
  GlobalSelectionSystem().foreachSelected(BrushVisibleSelectedVisitor<Functor>(functor));
  return functor;
}

class BrushForEachFace
{
  const BrushInstanceVisitor& m_visitor;
public:
  BrushForEachFace(const BrushInstanceVisitor& visitor) : m_visitor(visitor)
  {
  }
  void operator()(BrushInstance& brush) const
  {
    brush.forEachFaceInstance(m_visitor);
  }
};

template<class Functor>
class FaceInstanceVisitFace : public BrushInstanceVisitor
{
  const Functor& functor;
public:
  FaceInstanceVisitFace(const Functor& functor)
    : functor(functor)
  {
  }
  void visit(FaceInstance& face) const
  {
    functor(face.getFace());
  }
};

template<typename Functor>
inline const Functor& Brush_forEachFace(BrushInstance& brush, const Functor& functor)
{
  brush.forEachFaceInstance(FaceInstanceVisitFace<Functor>(functor));
  return functor;
}

template<class Functor>
class FaceVisitAll : public BrushVisitor
{
  const Functor& functor;
public:
  FaceVisitAll(const Functor& functor)
    : functor(functor)
  {
  }
  void visit(Face& face) const
  {
    functor(face);
  }
};

template<typename Functor>
inline const Functor& Brush_forEachFace(const Brush& brush, const Functor& functor)
{
  brush.forEachFace(FaceVisitAll<Functor>(functor));
  return functor;
}

template<typename Functor>
inline const Functor& Brush_forEachFace(Brush& brush, const Functor& functor)
{
  brush.forEachFace(FaceVisitAll<Functor>(functor));
  return functor;
}

template<class Functor>
class FaceInstanceVisitAll : public BrushInstanceVisitor
{
  const Functor& functor;
public:
  FaceInstanceVisitAll(const Functor& functor)
    : functor(functor)
  {
  }
  void visit(FaceInstance& face) const
  {
    functor(face);
  }
};

template<typename Functor>
inline const Functor& Brush_ForEachFaceInstance(BrushInstance& brush, const Functor& functor)
{
  brush.forEachFaceInstance(FaceInstanceVisitAll<Functor>(functor));
  return functor;
}

template<typename Functor>
inline const Functor& Scene_forEachBrush(scene::Graph& graph, const Functor& functor)
{
  graph.traverse(InstanceWalker< InstanceApply<BrushInstance, Functor> >(functor));
  return functor;
}

template<typename Type, typename Functor>
class InstanceIfVisible : public Functor
{
public:
  InstanceIfVisible(const Functor& functor) : Functor(functor)
  {
  }
  void operator()(scene::Instance& instance)
  {
    if(instance.path().top().get().visible())
    {
      Functor::operator()(instance);
    }
  }
};

template<typename Functor>
class BrushVisibleWalker : public scene::Graph::Walker
{
  const Functor& m_functor;
public:
  BrushVisibleWalker(const Functor& functor) : m_functor(functor)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    if(path.top().get().visible())
    {
      BrushInstance* brush = Instance_getBrush(instance);
      if(brush != 0)
      {
        m_functor(*brush);
      }
    }
    return true;
  }
};

template<typename Functor>
inline const Functor& Scene_forEachVisibleBrush(scene::Graph& graph, const Functor& functor)
{
  graph.traverse(BrushVisibleWalker<Functor>(functor));
  return functor;
}

template<typename Functor>
inline const Functor& Scene_ForEachBrush_ForEachFace(scene::Graph& graph, const Functor& functor)
{
  Scene_forEachBrush(graph, BrushForEachFace(FaceInstanceVisitFace<Functor>(functor)));
  return functor;
}

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrush_ForEachFace(scene::Graph& graph, const Functor& functor)
{
  Scene_forEachSelectedBrush(BrushForEachFace(FaceInstanceVisitFace<Functor>(functor)));
  return functor;
}

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrush_ForEachFaceInstance(scene::Graph& graph, const Functor& functor)
{
  Scene_forEachSelectedBrush(BrushForEachFace(FaceInstanceVisitAll<Functor>(functor)));
  return functor;
}

template<typename Functor>
class FaceVisitorWrapper
{
  const Functor& functor;
public:
  FaceVisitorWrapper(const Functor& functor) : functor(functor)
  {
  }

  void operator()(FaceInstance& faceInstance) const
  {
    functor(faceInstance.getFace());
  }
};

template<typename Functor>
inline const Functor& Scene_ForEachSelectedBrushFace(scene::Graph& graph, const Functor& functor)
{
  g_SelectedFaceInstances.foreach(FaceVisitorWrapper<Functor>(functor));
  return functor;
}


#endif
