#include "RenderableBrushVertices.h"

#include "Brush.h"

namespace brush
{

void RenderableBrushVertices::updateGeometry()
{
    if (!_updateNeeded) return;

    _updateNeeded = false;

    // Get the vertices for our given mode
    const auto& brushVertices = _brush.getVertices(_mode);

    std::vector<ArbitraryMeshVertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(brushVertices.size());
    indices.reserve(brushVertices.size());

    for (auto i = 0; i < brushVertices.size(); ++i)
    {
        const auto& vertex = brushVertices[i];

        Vector4 colour(
            vertex.colour.r / 255.0,
            vertex.colour.g / 255.0,
            vertex.colour.b / 255.0,
            1.0
        );

        vertices.push_back(ArbitraryMeshVertex(vertex.vertex, { 0,0,0 }, { 0,0 }, colour));
        indices.push_back(i);
    }

    RenderableGeometry::updateGeometry(render::GeometryType::Points, vertices, indices);
}

}
