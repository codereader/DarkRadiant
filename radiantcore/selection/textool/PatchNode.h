#pragma once

#include "ipatch.h"
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
        _patch.undoSave();
    }

    void revertTransformation() override
    {
        _patch.revertTransform();
        _patch.controlPointsChanged();
    }

    void applyTransformToSelected(const Matrix3& transform) override
    {
        foreachVertex([&](PatchControl& vertex)
        {
            vertex.texcoord = transform * vertex.texcoord;
        });

        _patch.controlPointsChanged();
    }

    void commitTransformation() override
    {
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

        foreachVertex([&](PatchControl& vertex)
        {
            SelectionIntersection intersection;

            test.TestPoint(Vector3(vertex.texcoord.x(), vertex.texcoord.y(), 0), intersection);

            if (intersection.isValid())
            {
                Selector_add(selector, *this);
            }
        });
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
                functor(_patch.ctrlAt(row, col));
            }
        }
    }
};

}
