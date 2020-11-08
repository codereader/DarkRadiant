#pragma once

#include "ipatch.h"
#include "imodelsurface.h"

namespace model
{

// Adapter methods to convert patch vertices to ArbitraryMeshVertex type
inline ArbitraryMeshVertex convertPatchVertex(const VertexNT& in)
{
    // Colour will be set to 1,1,1 by the constructor
    return ArbitraryMeshVertex(in.vertex, in.normal, in.texcoord);
}

/**
 * Adapter class converting a PatchTesselation object into
 * a surface suitable for exporting it to a model file.
 * The PatchSurface class will generate the triangle indices
 * (clockwise) as expeced by the IModelExporter interface.
 */
class PatchSurface : 
    public IIndexedModelSurface
{
private:
    std::vector<ArbitraryMeshVertex> _vertices;
    std::vector<unsigned int> _indices;
    std::string _materialName;

public:
    PatchSurface(const std::string& materialName, PatchMesh& mesh) :
        _materialName(materialName)
    {
        assert(mesh.width >= 2 && mesh.height >= 2);

        _vertices.reserve(mesh.vertices.size());
        _indices.reserve((mesh.height - 1) * (mesh.width- 1) * 6); // 6 => 2 triangles per quad

        // Load all the vertices into the target vector
        std::transform(mesh.vertices.begin(), mesh.vertices.end(),
            std::back_inserter(_vertices), convertPatchVertex);

        // Generate the indices to define the triangles in clockwise order
        for (std::size_t h = 0; h < mesh.height - 1; ++h)
        {
            auto rowOffset = h * mesh.width;

            for (std::size_t w = 0; w < mesh.width - 1; ++w)
            {
                _indices.push_back(rowOffset + w + mesh.width);
                _indices.push_back(rowOffset + w + 1);
                _indices.push_back(rowOffset + w);

                _indices.push_back(rowOffset + w + mesh.width);
                _indices.push_back(rowOffset + w + mesh.width + 1);
                _indices.push_back(rowOffset + w + 1);
            }
        }
    }

    // Inherited via IIndexedModelSurface
    int getNumVertices() const override
    {
        return static_cast<int>(_vertices.size());
    }

    int getNumTriangles() const override
    {
        return static_cast<int>(_indices.size() / 3); // 3 indices per triangle
    }

    const ArbitraryMeshVertex& getVertex(int vertexNum) const override
    {
        return _vertices[vertexNum];
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
        return _materialName;
    }

    const std::string& getActiveMaterial() const override
    {
        return _materialName;
    }

    const std::vector<ArbitraryMeshVertex>& getVertexArray() const override
    {
        return _vertices;
    }

    const std::vector<unsigned int>& getIndexArray() const override
    {
        return _indices;
    }
};

}
