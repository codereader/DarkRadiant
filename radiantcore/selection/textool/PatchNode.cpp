#include "PatchNode.h"

#include "ipatch.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <GL/glew.h>

namespace textool
{

PatchNode::PatchNode(IPatch& patch) :
    _patch(patch)
{
    foreachVertex([&](PatchControl& vertex)
    {
        _vertices.emplace_back(vertex.vertex, vertex.texcoord);
    });
}

IPatch& PatchNode::getPatch()
{
    return _patch;
}

void PatchNode::beginTransformation()
{
    // We call undoSave() here for consistency, but technically it's too early -
    // the undo operation hasn't started yet
    _patch.undoSave();
}

void PatchNode::revertTransformation()
{
    _patch.revertTransform();
    _patch.updateTesselation();
}

void PatchNode::transform(const Matrix3& transform)
{
    foreachVertex([&](PatchControl& vertex)
    {
        vertex.texcoord = transform * vertex.texcoord;
    });

    // We have to force the patch to update its tesselation since
    // modifying the "transformed" control point set won't trigger this
    _patch.updateTesselation(true);
}

void PatchNode::transformComponents(const Matrix3& transform)
{
    for (auto& vertex : _vertices)
    {
        if (!vertex.isSelected()) continue;

        vertex.getTexcoord() = transform * vertex.getTexcoord();
    }

    // We have to force the patch to update its tesselation since
    // modifying the "transformed" control point set won't trigger this
    _patch.updateTesselation(true);
}

void PatchNode::commitTransformation()
{
    // Patch::freezeTransform will call undoSave() before overwriting
    // the control point set with the transformed ones
    _patch.freezeTransform();
}

const AABB& PatchNode::localAABB() const
{
    _bounds = AABB();

    foreachVertex([&](PatchControl& vertex)
    {
        _bounds.includePoint({ vertex.texcoord.x(), vertex.texcoord.y(), 0 });
    });

    return _bounds;
}

void PatchNode::testSelect(Selector& selector, SelectionTest& test)
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
}

void PatchNode::render(SelectionMode mode)
{
    glEnable(GL_BLEND);
    glBlendColor(0, 0, 0, 0.3f);
    glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

    auto surfaceColour = getSurfaceColour(mode);
    glColor4fv(surfaceColour);

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

    if (mode == SelectionMode::Vertex)
    {
        renderComponents();
    }
}

void PatchNode::expandSelectionToRelated()
{}

void PatchNode::snapto(float snap)
{
    for (auto& vertex : _vertices)
    {
        auto& texcoord = vertex.getTexcoord();
        texcoord.x() = float_snapped(texcoord.x(), snap);
        texcoord.y() = float_snapped(texcoord.y(), snap);
    }

    _patch.updateTesselation(true);
}

void PatchNode::snapComponents(float snap)
{
    for (auto& vertex : _vertices)
    {
        if (vertex.isSelected())
        {
            auto& texcoord = vertex.getTexcoord();
            texcoord.x() = float_snapped(texcoord.x(), snap);
            texcoord.y() = float_snapped(texcoord.y(), snap);
        }
    }

    _patch.updateTesselation(true);
}

void PatchNode::mergeComponentsWith(const Vector2& center)
{
    for (auto& vertex : _vertices)
    {
        if (vertex.isSelected())
        {
            vertex.getTexcoord() = center;
        }
    }

    _patch.updateTesselation(true);
}

void PatchNode::foreachVertex(const std::function<void(PatchControl&)>& functor) const
{
    for (std::size_t col = 0; col < _patch.getWidth(); ++col)
    {
        for (std::size_t row = 0; row < _patch.getHeight(); ++row)
        {
            functor(_patch.getTransformedCtrlAt(row, col));
        }
    }
}

}
