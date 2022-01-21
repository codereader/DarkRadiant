#pragma once

#include "render/RenderableGeometry.h"

namespace map
{

namespace detail
{

inline Vector4 toVector4(const Colour4b& colour)
{
    return
    {
        colour.r / 255.0,
        colour.g / 255.0,
        colour.b / 255.0,
        colour.a / 255.0
    };
}

}

class RenderablePointFile :
    public render::RenderableGeometry
{
private:
    const std::vector<VertexCb>& _points;

public:
    RenderablePointFile(const std::vector<VertexCb>& points) :
        _points(points)
    {}

protected:
    void updateGeometry() override
    {
        std::vector<ArbitraryMeshVertex> vertices;
        std::vector<unsigned int> indices;

        if (_points.size() < 2) return;

        for (unsigned int i = 0; i < _points.size(); ++i)
        {
            vertices.push_back(ArbitraryMeshVertex(_points[i].vertex, { 0, 0, 0 }, { 0, 0 }, detail::toVector4(_points[i].colour)));

            if (i > 0)
            {
                indices.push_back(i-1);
                indices.push_back(i);
            }
        }
        
        RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices, indices);
    }
};

}
