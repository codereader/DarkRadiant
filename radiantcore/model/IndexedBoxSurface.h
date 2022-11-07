#pragma once

#include "RenderableModelSurface.h"
#include "render/RenderableBox.h"

namespace model
{

class IndexedBoxSurface final :
    public IIndexedModelSurface
{
private:
    const AABB& _bounds;
    const Matrix4& _orientation;

    std::vector<MeshVertex> _vertices;
    std::vector<unsigned int> _indices;

public:
    IndexedBoxSurface(const AABB& bounds, const Matrix4& orientation) :
        _bounds(bounds),
        _orientation(orientation)
    {
        static Vector3 Origin(0, 0, 0);

        // Calculate the corner vertices of this bounding box
        Vector3 max(Origin + _bounds.extents);
        Vector3 min(Origin - _bounds.extents);

        auto vertices = render::detail::getFillBoxVertices(min, max, { 1, 1, 1, 1 });

        for (const auto& vertex : vertices)
        {
            _vertices.push_back(MeshVertex(
                toVector3(vertex.vertex),
                toVector3(vertex.normal),
                Vector2(vertex.texcoord.x(), vertex.texcoord.y()),
                Vector4(vertex.colour.x(), vertex.colour.y(), vertex.colour.z(), vertex.colour.w()),
                toVector3(vertex.tangent),
                toVector3(vertex.bitangent)
            ));
        }

        _indices = render::detail::generateTriangleBoxIndices();
    }

    int getNumVertices() const override
    {
        return static_cast<int>(_vertices.size());
    }

    int getNumTriangles() const override
    {
        return static_cast<int>(_indices.size() / 3); // 3 indices per triangle
    }

    const MeshVertex& getVertex(int vertexNum) const override
    {
        return _vertices.at(vertexNum);
    }

    ModelPolygon getPolygon(int polygonIndex) const override
    {
        assert(polygonIndex >= 0 && polygonIndex * 3 < static_cast<int>(_indices.size()));

        ModelPolygon poly;

        poly.a = _vertices[_indices[polygonIndex * 3]];
        poly.b = _vertices[_indices[polygonIndex * 3 + 1]];
        poly.c = _vertices[_indices[polygonIndex * 3 + 2]];

        return poly;
    }

    const std::string& getDefaultMaterial() const override
    {
        static std::string _defaultMaterial;
        return _defaultMaterial;
    }

    const std::string& getActiveMaterial() const override
    {
        return getDefaultMaterial();
    }

    const AABB& getSurfaceBounds() const override
    {
        return _bounds;
    }

    const std::vector<MeshVertex>& getVertexArray() const override
    {
        return _vertices;
    }

    const std::vector<unsigned int>& getIndexArray() const override
    {
        return _indices;
    }

private:
    inline static Vector3 toVector3(const Vector3f& vector)
    {
        return { vector.x(), vector.y(), vector.z() };
    }
};

}
