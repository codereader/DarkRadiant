#pragma once

#include "ibrush.h"
#include "NodeBase.h"
#include "math/Matrix3.h"
#include "SelectableVertex.h"

namespace textool
{

class FaceNode :
    public NodeBase,
    public IFaceNode
{
private:
    IFace& _face;
    mutable AABB _bounds;

public:
    FaceNode(IFace& face) :
        _face(face)
    {
        for (auto& vertex : _face.getWinding())
        {
            _vertices.emplace_back(vertex.vertex, vertex.texcoord);
        }
    }

    IFace& getFace() override
    {
        return _face;
    }

    void beginTransformation() override
    {
        _face.undoSave();
    }

    void revertTransformation() override
    {
        _face.revertTransform();
    }

    void transform(const Matrix3& transform) override
    {
        for (auto& vertex : _face.getWinding())
        {
            vertex.texcoord = transform * vertex.texcoord;
        }

        Vector3 vertices[3] = { _face.getWinding().at(0).vertex, _face.getWinding().at(1).vertex, _face.getWinding().at(2).vertex };
        Vector2 texcoords[3] = { _face.getWinding().at(0).texcoord, _face.getWinding().at(1).texcoord, _face.getWinding().at(2).texcoord };

        _face.setTexDefFromPoints(vertices, texcoords);
    }

    void transformComponents(const Matrix3& transform) override
    {
        std::vector<std::size_t> selectedIndices;

        // Manipulate every selected vertex using the given transform
        for (std::size_t i = 0; i < _vertices.size(); ++i)
        {
            auto& vertex = _vertices[i];

            if (!vertex.isSelected()) continue;

            selectedIndices.push_back(i);
            vertex.getTexcoord() = transform * vertex.getTexcoord();
        }

        // Now we need to pick three vertices to calculate the tex def from
        // we have certain options, depending on the number of selected vertices
        auto selectionCount = selectedIndices.size();

        Vector3 vertices[3];
        Vector2 texcoords[3];
        const auto& winding = _face.getWinding();

        if (selectionCount >= 3)
        {
            // Manipulating 3+ vertices means that the whole face is transformed 
            // the same way. We can pick any of the three selected vertices.
            for (std::size_t i = 0; i < 3; ++i)
            {
                vertices[i] = _vertices[selectedIndices[i]].getVertex();
                texcoords[i] = _vertices[selectedIndices[i]].getTexcoord();
            }

            _face.setTexDefFromPoints(vertices, texcoords);
        }
        else if (selectionCount == 2)
        {
            // Calculate the center point of the selection and pick the vertex that is farthest from it
            auto selectionBounds = getSelectedComponentBounds();
            auto farthestIndex = findIndexFarthestFrom({ selectionBounds.origin.x(), selectionBounds.origin.y() });

            for (std::size_t i = 0; i < 2; ++i)
            {
                vertices[i] = _vertices[selectedIndices[i]].getVertex();
                texcoords[i] = _vertices[selectedIndices[i]].getTexcoord();
            }

            vertices[2] = _vertices[farthestIndex].getVertex();
            texcoords[2] = _vertices[farthestIndex].getTexcoord();

            _face.setTexDefFromPoints(vertices, texcoords);
        }
    }

    void commitTransformation() override
    {
        _face.freezeTransform();
    }

    const AABB& localAABB() const
    {
        _bounds = AABB();

        for (const auto& vertex : _face.getWinding())
        {
            _bounds.includePoint({ vertex.texcoord.x(), vertex.texcoord.y(), 0 });
        }

        return _bounds;
    }

    void testSelect(Selector& selector, SelectionTest& test) override
    {
        // Arrange the UV coordinates in a Vector3 array for testing
        std::vector<Vector3> uvs;
        uvs.reserve(_face.getWinding().size());

        for (const auto& vertex : _face.getWinding())
        {
            uvs.emplace_back(vertex.texcoord.x(), vertex.texcoord.y(), 0);
        }

        test.BeginMesh(Matrix4::getIdentity(), true);

        SelectionIntersection best;
        test.TestPolygon(VertexPointer(uvs.data(), sizeof(Vector3)), uvs.size(), best);

        if (best.isValid())
        {
            Selector_add(selector, *this);
        }
    }

    void render(SelectionMode mode) override
    {
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.3f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        if (mode == SelectionMode::Surface && isSelected())
        {
            glColor3f(1, 0.5f, 0);
        }
        else if (mode == SelectionMode::Vertex)
        {
            glColor3f(0.6f, 0.6f, 0.6f);
        }
        else
        {
            glColor3f(0.8f, 0.8f, 0.8f);
        }

        glBegin(GL_TRIANGLE_FAN);

        for (const auto& vertex : _face.getWinding())
        {
            glVertex2d(vertex.texcoord[0], vertex.texcoord[1]);
        }

        glEnd();
        glDisable(GL_BLEND);

        if (mode == SelectionMode::Vertex)
        {
            renderComponents();
        }
    }

private:
    // Locates the index of the unselected vertex that is farthest away from the given texcoord
    std::size_t findIndexFarthestFrom(const Vector2& texcoord)
    {
        assert(!_vertices.empty());

        std::size_t farthestIndex = 0;
        double largestDistanceSquared = 0;

        for (std::size_t i = 0; i < _vertices.size(); ++i)
        {
            if (_vertices[i].isSelected()) continue;

            auto candidateDistanceSquared = (_vertices[i].getTexcoord() - texcoord).getLengthSquared();

            if (candidateDistanceSquared > largestDistanceSquared)
            {
                farthestIndex = i;
                largestDistanceSquared = candidateDistanceSquared;
            }
        }

        return farthestIndex;
    }
};

}
