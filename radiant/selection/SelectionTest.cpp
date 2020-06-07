#include "SelectionTest.h"

#include "igroupnode.h"
#include "itextstream.h"
#include "entitylib.h"
#include "imodel.h"
#include "debugging/ScenegraphUtils.h"

inline SelectionIntersection select_point_from_clipped(Vector4& clipped) {
  return SelectionIntersection(clipped[2] / clipped[3], static_cast<float>(Vector3(clipped[0] / clipped[3], clipped[1] / clipped[3], 0).getLengthSquared()));
}

void SelectionVolume::BeginMesh(const Matrix4& localToWorld, bool twoSided)
{
    _local2view = _view.GetViewMatrix().getMultipliedBy(localToWorld);

    // Cull back-facing polygons based on winding being clockwise or counter-clockwise.
    // Don't cull if the view is wireframe and the polygons are two-sided.
    _cull = twoSided && !_view.fill() ? eClipCullNone : (localToWorld.getHandedness() == Matrix4::RIGHTHANDED) ? eClipCullCW : eClipCullCCW;

    {
      Matrix4 screen2world(_local2view.getFullInverse());

      _near = screen2world.transformPoint(Vector3(0, 0, -1));
	  _far = screen2world.transformPoint(Vector3(0, 0, 1));
    }
}

void SelectionVolume::TestPoint(const Vector3& point, SelectionIntersection& best) {
    Vector4 clipped;
    if (_local2view.clipPoint(point, clipped) == c_CLIP_PASS)
    {
      best = select_point_from_clipped(clipped);
    }
}

void SelectionVolume::TestPolygon(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) {
    Vector4 clipped[9];
    for(std::size_t i=0; i+2<count; ++i)
    {
      BestPoint(
        _local2view.clipTriangle(
          vertices[0],
          vertices[i+1],
          vertices[i+2],
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestLineLoop(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) {
    if(count == 0)
      return;
    Vector4 clipped[9];
    for(VertexPointer::iterator i = vertices.begin(), end = i + count, prev = i + (count-1); i != end; prev = i, ++i)
    {
      BestPoint(
        _local2view.clipLine(
          *prev,
          *i,
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestLineStrip(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) {
    if(count == 0)
      return;
    Vector4 clipped[9];
    for(VertexPointer::iterator i = vertices.begin(), end = i + count, next = i + 1; next != end; i = next, ++next)
    {
      BestPoint(
        _local2view.clipLine(
          *i,
          *next,
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestLines(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) {
    if(count == 0)
      return;
    Vector4 clipped[9];
    for(VertexPointer::iterator i = vertices.begin(), end = i + count; i != end; i += 2)
    {
      BestPoint(
        _local2view.clipLine(
          *i,
          *(i+1),
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestTriangles(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) {
    Vector4 clipped[9];
    for(IndexPointer::iterator i(indices.begin()); i != indices.end(); i += 3)
    {
      BestPoint(
        _local2view.clipTriangle(
          vertices[*i],
          vertices[*(i+1)],
          vertices[*(i+2)],
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestQuads(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) {
    Vector4 clipped[9];
    for(IndexPointer::iterator i(indices.begin()); i != indices.end(); i += 4)
    {
      BestPoint(
		  _local2view.clipTriangle(
          vertices[*i],
          vertices[*(i+1)],
          vertices[*(i+3)],
          clipped
        ),
        clipped,
        best,
        _cull
      );
	    BestPoint(
        _local2view.clipTriangle(
          vertices[*(i+1)],
          vertices[*(i+2)],
          vertices[*(i+3)],
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}

void SelectionVolume::TestQuadStrip(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) {
    Vector4 clipped[9];
    for(IndexPointer::iterator i(indices.begin()); i+2 != indices.end(); i += 2)
    {
      BestPoint(
        _local2view.clipTriangle(
          vertices[*i],
          vertices[*(i+1)],
          vertices[*(i+2)],
          clipped
        ),
        clipped,
        best,
        _cull
      );
      BestPoint(
        _local2view.clipTriangle(
          vertices[*(i+2)],
          vertices[*(i+1)],
          vertices[*(i+3)],
          clipped
        ),
        clipped,
        best,
        _cull
      );
    }
}
