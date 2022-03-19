#pragma once

#include <vector>
#include "math/Vector4.h"
#include "math/AABB.h"
#include "RenderableBox.h"
#include "RenderableGeometry.h"

namespace render
{

class RenderableBoundingBoxes :
    public render::RenderableGeometry
{
private:
    const std::vector<AABB>& _aabbs;
    bool _needsUpdate;
    Vector4 _colour;

public:
    RenderableBoundingBoxes(const std::vector<AABB>& aabbs, const Vector4& colour = { 1,1,1,1 }) :
        _aabbs(aabbs),
        _needsUpdate(true),
        _colour(colour)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    virtual Vector4 getBoxColour(std::size_t boxIndex)
    {
        return _colour;
    }

    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        std::vector<RenderVertex> vertices;
        std::vector<unsigned int> indices;

        static auto WireframeBoxIndices = detail::generateWireframeBoxIndices();

        vertices.reserve(_aabbs.size() * 8); // 8 vertices per box
        indices.reserve(WireframeBoxIndices.size() * _aabbs.size()); // indices per box * boxes

        for (auto i = 0; i < _aabbs.size(); ++i)
        {
            const auto& aabb = _aabbs.at(i);

            // Calculate the corner vertices of this bounding box
            Vector3 max(aabb.origin + aabb.extents);
            Vector3 min(aabb.origin - aabb.extents);

            auto boxVertices = detail::getWireframeBoxVertices(min, max, getBoxColour(i));

            auto indexOffset = static_cast<unsigned int>(vertices.size());

            vertices.insert(vertices.begin(),
                std::make_move_iterator(boxVertices.begin()),
                std::make_move_iterator(boxVertices.end()));

            for (auto index : WireframeBoxIndices)
            {
                indices.push_back(indexOffset + index);
            }
        }

        updateGeometryWithData(render::GeometryType::Lines, vertices, indices);
    }
};

}
