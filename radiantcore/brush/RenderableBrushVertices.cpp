#include "RenderableBrushVertices.h"

#include "ibrush.h"
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

    static const Vector3& vertexColour = GlobalBrushCreator().getSettings().getVertexColour();
    const Vector4 colour(vertexColour, 1);

    for (auto i = 0; i < brushVertices.size(); ++i)
    {
        const auto& vertex = brushVertices[i];

        vertices.push_back(ArbitraryMeshVertex(vertex, { 0,0,0 }, { 0,0 }, colour));
        indices.push_back(i);
    }

    RenderableGeometry::updateGeometry(render::GeometryType::Points, vertices, indices);
}

}
