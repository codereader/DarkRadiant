#pragma once

#include <vector>
#include "irender.h"
#include "Winding.h"

namespace render
{

class RenderableWinding
{
private:
    const Winding& _winding;
    bool _needsUpdate;

public:
    RenderableWinding(const Winding& winding) :
        _winding(winding),
        _needsUpdate(true)
    {}

    void update(const ShaderPtr& shader)
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        auto numPoints = _winding.size();
        if (numPoints < 3) return;

        // Triangulate our winding and submit
        std::vector<unsigned int> indices;
        indices.reserve(3 * (numPoints - 2));

        std::vector<ArbitraryMeshVertex> vertices;
        vertices.reserve(numPoints);

        for (const auto& vertex : _winding)
        {
            vertices.emplace_back(ArbitraryMeshVertex(vertex.vertex, vertex.normal, vertex.texcoord, { 1, 1, 1 }));
        }

        for (unsigned int n = static_cast<unsigned int>(numPoints) - 1; n - 1 > 0; --n)
        {
            indices.push_back(0);
            indices.push_back(n - 1);
            indices.push_back(n);
        }

        shader->addSurface(vertices, indices);
    }
};

}
