#pragma once

#include <vector>
#include "irenderable.h"
#include "render.h"
#include "render/RenderableGeometry.h"

namespace entity
{

class RenderableCurve :
    public render::RenderableGeometry
{
private:
    const IEntityNode& _entity;
    bool _needsUpdate;

public:
	std::vector<VertexCb> m_vertices;

    RenderableCurve(const IEntityNode& entity) :
        _entity(entity),
        _needsUpdate(true)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        if (m_vertices.size() < 2)
        {
            clear();
            return;
        }

        std::vector<render::RenderVertex> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve(m_vertices.size());
        indices.reserve(m_vertices.size() << 1);

        unsigned int index = 0;

        auto colour = _entity.getEntityColour();

        for (const auto& v : m_vertices)
        {
            vertices.push_back(render::RenderVertex(v.vertex, { 0,0,1 }, { 0,0 }, colour));
            indices.push_back(index);
            indices.push_back(++index);
        };

        // Remove the last two superfluous indices
        indices.pop_back();
        indices.pop_back();

        updateGeometryWithData(render::GeometryType::Lines, vertices, indices);
    }
};

} // namespace entity
