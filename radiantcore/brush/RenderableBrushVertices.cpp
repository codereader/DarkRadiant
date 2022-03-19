#include "RenderableBrushVertices.h"

#include "ibrush.h"
#include "Brush.h"

namespace brush
{

namespace detail
{

inline void addColouredVertices(const std::vector<Vector3>& sourceVertices, const Vector4& colour,
    std::vector<render::RenderVertex>& vertices, std::vector<unsigned int>& indices)
{
    auto indexOffset = static_cast<unsigned int>(vertices.size());

    for (unsigned int i = 0; i < sourceVertices.size(); ++i)
    {
        const auto& vertex = sourceVertices[i];

        vertices.push_back(render::RenderVertex(vertex, { 0,0,0 }, { 0,0 }, colour));
        indices.push_back(indexOffset + i);
    }
}

}

void RenderableBrushVertices::updateGeometry()
{
    if (!_updateNeeded) return;

    _updateNeeded = false;

    // Get the vertices for our given mode
    const auto& brushVertices = _brush.getVertices(_mode);

    std::vector<render::RenderVertex> vertices;
    std::vector<unsigned int> indices;

    auto totalSize = brushVertices.size() + _selectedVertices.size();
    vertices.reserve(totalSize);
    indices.reserve(totalSize);

    static const Vector3& vertexColour = GlobalBrushCreator().getSettings().getVertexColour();
    static const Vector3& selectedVertexColour = GlobalBrushCreator().getSettings().getSelectedVertexColour();

    detail::addColouredVertices(brushVertices, { vertexColour, 1 }, vertices, indices);
    detail::addColouredVertices(_selectedVertices, { selectedVertexColour, 1 }, vertices, indices);

    updateGeometryWithData(render::GeometryType::Points, vertices, indices);
}

}
