#pragma once

#include "ipatch.h"
#include "imodelsurface.h"

namespace model
{

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
    AABB _bounds;

public:
    PatchSurface(const std::string& materialName, PatchMesh& mesh);

    int getNumVertices() const override;
    int getNumTriangles() const override;

    const ArbitraryMeshVertex& getVertex(int vertexNum) const override;
    ModelPolygon getPolygon(int polygonIndex) const override;

    const std::string& getDefaultMaterial() const override;
    const std::string& getActiveMaterial() const override;

    const std::vector<ArbitraryMeshVertex>& getVertexArray() const override;
    const std::vector<unsigned int>& getIndexArray() const override;

    const AABB& getSurfaceBounds() const override;
};

}
