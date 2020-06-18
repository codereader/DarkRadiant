#pragma once

#include "iscenegraph.h"
#include "Rectangle.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "iselectiontest.h"

#include "render/View.h"
#include "selection/BestPoint.h"

class SelectionVolume : public SelectionTest {
  Matrix4 _local2view;
  render::View _view;
  clipcull_t _cull;
  Vector3 _near;
  Vector3 _far;
public:
  SelectionVolume(const render::View& view): _view(view) {}

  const VolumeTest& getVolume() const {
    return _view;
  }

  const Vector3& getNear() const {
    return _near;
  }

  const Vector3& getFar() const {
    return _far;
  }

  void BeginMesh(const Matrix4& localToWorld, bool twoSided);
  void TestPoint(const Vector3& point, SelectionIntersection& best);
  void TestPolygon(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best);
  void TestLineLoop(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best);
  void TestLineStrip(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best);
  void TestLines(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best);
  void TestTriangles(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best);
  void TestQuads(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best);
  void TestQuadStrip(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best);
};

// --------------------------------------------------------------------------------

inline void ConstructSelectionTest(render::View& view, const selection::Rectangle& selection_box)
{
	view.EnableScissor(selection_box.min[0], selection_box.max[0],
					   selection_box.min[1], selection_box.max[1]);
}
