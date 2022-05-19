#pragma once

#include "iscenegraph.h"
#include "../Rectangle.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "iselectiontest.h"

#include "render/View.h"
#include "BestPoint.h"

inline SelectionIntersection select_point_from_clipped(Vector4& clipped)
{
    return SelectionIntersection(static_cast<float>(clipped[2] / clipped[3]),
        static_cast<float>(Vector3(clipped[0] / clipped[3], clipped[1] / clipped[3], 0).getLengthSquared()));
}

class SelectionVolume :
    public SelectionTest
{
    Matrix4 _local2view;
    render::View _view;
    clipcull_t _cull;
    Vector3 _near;
    Vector3 _far;
public:
    SelectionVolume(const render::View& view) :
        _view(view)
    {}

    const VolumeTest& getVolume() const override
    {
        return _view;
    }

    const Vector3& getNear() const override
    {
        return _near;
    }

    const Vector3& getFar() const override
    {
        return _far;
    }

    void BeginMesh(const Matrix4& localToWorld, bool twoSided) override
    {
        _local2view = _view.GetViewProjection().getMultipliedBy(localToWorld);

        // Cull back-facing polygons based on winding being clockwise or counter-clockwise.
        // Don't cull if the material is twosided or the view is wireframe
        _cull = twoSided || !_view.fill() ? eClipCullNone :
            (localToWorld.getHandedness() == Matrix4::RIGHTHANDED) ? eClipCullCW : eClipCullCCW;

        Matrix4 screen2world(_local2view.getFullInverse());

        _near = screen2world.transformPoint(Vector3(0, 0, -1));
        _far = screen2world.transformPoint(Vector3(0, 0, 1));
    }

    void TestPoint(const Vector3& point, SelectionIntersection& best) override
    {
        Vector4 clipped;
        if (clipPoint(_local2view, point, clipped) == c_CLIP_PASS)
        {
            best = select_point_from_clipped(clipped);
        }
    }

    void TestPolygon(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) override
    {
        Vector4 clipped[9];
        for (std::size_t i = 0; i + 2 < count; ++i)
        {
            BestPoint(clipTriangle(_local2view, vertices[0], vertices[i + 1],
                                   vertices[i + 2], clipped),
                      clipped, best, _cull);
        }
    }

    void TestLineStrip(const VertexPointer& vertices, std::size_t count, SelectionIntersection& best) override
    {
        if (count == 0)
            return;
        Vector4 clipped[9];
        for (VertexPointer::iterator i = vertices.begin(), end = i + count, next = i + 1; next != end; i = next, ++next)
        {
            BestPoint(clipLine(_local2view, *i, *next, clipped), clipped, best,
                      _cull);
        }
    }

    void TestTriangles(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) override
    {
        Vector4 clipped[9];
        for (IndexPointer::iterator i(indices.begin()); i != indices.end(); i += 3)
        {
            BestPoint(clipTriangle(_local2view, vertices[*i],
                                   vertices[*(i + 1)], vertices[*(i + 2)],
                                   clipped),
                      clipped, best, _cull);
        }
    }

    void TestQuads(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) override
    {
        Vector4 clipped[9];
        for (IndexPointer::iterator i(indices.begin()); i != indices.end(); i += 4)
        {
            BestPoint(clipTriangle(_local2view, vertices[*i],
                                   vertices[*(i + 1)], vertices[*(i + 3)],
                                   clipped),
                      clipped, best, _cull);
            BestPoint(clipTriangle(_local2view, vertices[*(i + 1)],
                                   vertices[*(i + 2)], vertices[*(i + 3)],
                                   clipped),
                      clipped, best, _cull);
        }
    }

    void TestQuadStrip(const VertexPointer& vertices, const IndexPointer& indices, SelectionIntersection& best) override
    {
        Vector4 clipped[9];
        for (IndexPointer::iterator i(indices.begin()); i + 2 != indices.end(); i += 2)
        {
            BestPoint(clipTriangle(_local2view, vertices[*i],
                                   vertices[*(i + 1)], vertices[*(i + 2)],
                                   clipped),
                      clipped, best, _cull);
            BestPoint(clipTriangle(_local2view, vertices[*(i + 2)],
                                   vertices[*(i + 1)], vertices[*(i + 3)],
                                   clipped),
                      clipped, best, _cull);
        }
    }
};

// --------------------------------------------------------------------------------

inline void ConstructSelectionTest(render::View& view, const selection::Rectangle& selection_box)
{
	view.EnableScissor(selection_box.min[0], selection_box.max[0],
					   selection_box.min[1], selection_box.max[1]);
}
