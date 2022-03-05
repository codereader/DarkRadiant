#include "PatchSurface.h"

#include <algorithm>

namespace model
{

namespace
{
    // Adapter method to convert patch vertices to MeshVertex type
    inline MeshVertex convertPatchVertex(const VertexNT& in)
    {
        // Colour will be set to 1,1,1 by the constructor
        return MeshVertex(in.vertex, in.normal, in.texcoord);
    }
}

PatchSurface::PatchSurface(const std::string& materialName, PatchMesh& mesh) :
    _materialName(materialName)
{
    assert(mesh.width >= 2 && mesh.height >= 2);

    _vertices.reserve(mesh.vertices.size());
    _indices.reserve((mesh.height - 1) * (mesh.width - 1) * 6); // 6 => 2 triangles per quad

    // Load all the vertices into the target vector
    std::transform(mesh.vertices.begin(), mesh.vertices.end(),
        std::back_inserter(_vertices), convertPatchVertex);

    _bounds = AABB();

    // Accumulate the bounds
    for (const auto& vertex : _vertices)
    {
        _bounds.includePoint(vertex);
    }

    // Generate the indices to define the triangles in clockwise order
    for (std::size_t h = 0; h < mesh.height - 1; ++h)
    {
        auto rowOffset = h * mesh.width;

        for (std::size_t w = 0; w < mesh.width - 1; ++w)
        {
            _indices.push_back(static_cast<unsigned int>(rowOffset + w + mesh.width));
            _indices.push_back(static_cast<unsigned int>(rowOffset + w + 1));
            _indices.push_back(static_cast<unsigned int>(rowOffset + w));

            _indices.push_back(static_cast<unsigned int>(rowOffset + w + mesh.width));
            _indices.push_back(static_cast<unsigned int>(rowOffset + w + mesh.width + 1));
            _indices.push_back(static_cast<unsigned int>(rowOffset + w + 1));
        }
    }
}

// Inherited via IIndexedModelSurface
int PatchSurface::getNumVertices() const
{
    return static_cast<int>(_vertices.size());
}

int PatchSurface::getNumTriangles() const
{
    return static_cast<int>(_indices.size() / 3); // 3 indices per triangle
}

const MeshVertex& PatchSurface::getVertex(int vertexNum) const
{
    return _vertices[vertexNum];
}

ModelPolygon PatchSurface::getPolygon(int polygonIndex) const
{
    assert(polygonIndex >= 0 && polygonIndex * 3 < static_cast<int>(_indices.size()));

    ModelPolygon poly;

    poly.a = _vertices[_indices[polygonIndex * 3]];
    poly.b = _vertices[_indices[polygonIndex * 3 + 1]];
    poly.c = _vertices[_indices[polygonIndex * 3 + 2]];

    return poly;
}

const std::string& PatchSurface::getDefaultMaterial() const
{
    return _materialName;
}

const std::string& PatchSurface::getActiveMaterial() const
{
    return _materialName;
}

const std::vector<MeshVertex>& PatchSurface::getVertexArray() const
{
    return _vertices;
}

const std::vector<unsigned int>& PatchSurface::getIndexArray() const
{
    return _indices;
}

const AABB& PatchSurface::getSurfaceBounds() const
{
    return _bounds;
}

}
