#pragma once

#include "ipatch.h"
#include "itextstream.h"
#include "NodeBase.h"

namespace textool
{

class PatchNode :
    public NodeBase
{
private:
    IPatch& _patch;
    mutable AABB _bounds;

public:
    PatchNode(IPatch& patch) :
        _patch(patch)
    {}

    void beginTransformation() override
    {
        // We call undoSave() here for consistency, but technically it's too early -
        // the undo operation hasn't started yet
        _patch.undoSave();
    }

    void revertTransformation() override
    {
        _patch.revertTransform();
        _patch.updateTesselation();
    }

    void applyTransformToSelected(const Matrix3& transform) override
    {
        foreachVertex([&](PatchControl& vertex)
        {
            vertex.texcoord = transform * vertex.texcoord;
        });

        // We have to force the patch to update its tesselation since
        // modifying the "transformed" control point set won't trigger this
        _patch.updateTesselation(true);
    }

    void commitTransformation() override
    {
        // Patch::freezeTransform will call undoSave() before overwriting
        // the control point set with the transformed ones
        _patch.freezeTransform();
    }

    const AABB& localAABB() const
    {
        _bounds = AABB();

        foreachVertex([&](PatchControl& vertex)
        {
            _bounds.includePoint({ vertex.texcoord.x(), vertex.texcoord.y(), 0 });
        });

        return _bounds;
    }

    void testSelect(Selector& selector, SelectionTest& test) override
    {
        test.BeginMesh(Matrix4::getIdentity(), true);

        auto mesh = _patch.getTesselatedPatchMesh();
        auto indices = _patch.getRenderIndices();

        // Copy the UV coords to the XYZ part to be able to use the TestQuadStrip method
        for (auto& vertex : mesh.vertices)
        {
            vertex.vertex.set(vertex.texcoord.x(), vertex.texcoord.y(), 0);
        }

        SelectionIntersection best;
        auto* pIndex = &indices.indices.front();

        for (auto s = 0; s < indices.numStrips; s++) 
        {
            test.TestQuadStrip(VertexPointer(&mesh.vertices.front().vertex, sizeof(VertexNT)), IndexPointer(pIndex, indices.lenStrips), best);
            pIndex += indices.lenStrips;
        }

        if (best.isValid())
        {
            Selector_add(selector, *this);
        }
#if 0
        foreachVertex([&](PatchControl& vertex)
        {
            SelectionIntersection intersection;

            test.TestPoint(Vector3(vertex.texcoord.x(), vertex.texcoord.y(), 0), intersection);

            if (intersection.isValid())
            {
                Selector_add(selector, *this);
            }
        });
#endif
    }

    void render() override
    {
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.3f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        if (isSelected())
        {
            glColor3f(1, 0.5f, 0);
        }
        else 
        {
            glColor3f(0.8f, 0.8f, 0.8f);
        }

        // Get the tesselation and the first
        auto tess = _patch.getTesselatedPatchMesh();

        auto renderInfo = _patch.getRenderIndices();
        auto* strip_indices = &renderInfo.indices.front();

        for (std::size_t i = 0; i < renderInfo.numStrips; i++, strip_indices += renderInfo.lenStrips)
        {
            glBegin(GL_QUAD_STRIP);

            for (std::size_t offset = 0; offset < renderInfo.lenStrips; offset++)
            {
                // Retrieve the mesh vertex from the line strip
                auto& meshVertex = tess.vertices[*(strip_indices + offset)];
                glVertex2d(meshVertex.texcoord[0], meshVertex.texcoord[1]);
            }

            glEnd();
        }

        glDisable(GL_BLEND);
    }

private:
    void foreachVertex(const std::function<void(PatchControl&)>& functor) const
    {
        for (std::size_t col = 0; col < _patch.getWidth(); ++col)
        {
            for (std::size_t row = 0; row < _patch.getHeight(); ++row)
            {
                functor(_patch.getTransformedCtrlAt(row, col));
            }
        }
    }
};

}
