#pragma once

#include "render/RenderVertex.h"

namespace test
{

inline render::RenderVertex createNthVertex(int n, int id, std::size_t size)
{
    auto offset = static_cast<float>(n + size * id);

    return render::RenderVertex(
        { offset + 0.0f, offset + 0.5f, offset + 0.3f },
        { 0, 0, offset + 0.0f },
        { offset + 0.0f, -offset + 0.0f }
    );
}

inline std::vector<render::RenderVertex> generateVertices(int id, std::size_t size)
{
    std::vector<render::RenderVertex> vertices;

    for (int i = 0; i < size; ++i)
    {
        vertices.emplace_back(createNthVertex(i, id, size));
    }

    return vertices;
}

// Generates 3 indices per vertex, without any special meaning
inline std::vector<unsigned int> generateIndices(const std::vector<render::RenderVertex>& vertices)
{
    std::vector<unsigned int> indices;

    for (int i = 0; i < vertices.size(); ++i)
    {
        indices.emplace_back(i);
        indices.emplace_back(static_cast<unsigned int>((i + 1) % vertices.size()));
        indices.emplace_back(static_cast<unsigned int>((i + 2) % vertices.size()));
    }

    return indices;
}

}
