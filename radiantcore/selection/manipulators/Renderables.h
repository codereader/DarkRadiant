#pragma once

#include "math/AABB.h"
#include "render/RenderableGeometry.h"
#include "render/RenderableBox.h"
#include "render.h"

namespace selection
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
    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        std::vector<ArbitraryMeshVertex> vertices;
        std::vector<unsigned int> indices;

        static auto WireframeBoxIndices = render::detail::generateWireframeBoxIndices();

        vertices.reserve(_aabbs.size() * 8); // 8 vertices per box
        indices.reserve(WireframeBoxIndices.size() * _aabbs.size()); // indices per box * boxes

        for (const auto& aabb : _aabbs)
        {
            // Calculate the corner vertices of this bounding box
            Vector3 max(aabb.origin + aabb.extents);
            Vector3 min(aabb.origin - aabb.extents);

            auto boxVertices = render::detail::getWireframeBoxVertices(min, max, _colour);

            auto indexOffset = static_cast<unsigned int>(vertices.size());

            vertices.insert(vertices.begin(),
                std::make_move_iterator(boxVertices.begin()), 
                std::make_move_iterator(boxVertices.end()));

            for (auto index : WireframeBoxIndices)
            {
                indices.push_back(indexOffset + index);
            }
        }

        RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices, indices);
    }
};

class RenderableCornerPoints :
    public render::RenderableGeometry
{
private:
    const std::vector<AABB>& _aabbs;
    bool _needsUpdate;
    Vector4 _colour;

public:
    RenderableCornerPoints(const std::vector<AABB>& aabbs) :
        _aabbs(aabbs),
        _needsUpdate(true),
        _colour({ 1,1,1,1 })
    {}

    void setColour(const Colour4b& colour)
    {
        _colour.x() = colour.r / 255.0;
        _colour.y() = colour.g / 255.0;
        _colour.z() = colour.b / 255.0;
        _colour.w() = colour.a / 255.0;
    }

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        std::vector<ArbitraryMeshVertex> vertices;
        std::vector<unsigned int> indices;

        // 8 vertices per box
        vertices.reserve(_aabbs.size() * 8);
        indices.reserve(_aabbs.size() * 8);

        unsigned int index = 0;

        for (const auto& aabb : _aabbs)
        {
            // Calculate the corner vertices of this bounding box
            Vector3 max(aabb.origin + aabb.extents);
            Vector3 min(aabb.origin - aabb.extents);

            auto boxVertices = render::detail::getWireframeBoxVertices(min, max, _colour);
            
            for (const auto& vertex : boxVertices)
            {
                vertices.emplace_back(std::move(vertex));
                indices.push_back(index++);
            }
        }

        RenderableGeometry::updateGeometry(render::GeometryType::Points, vertices, indices);
    }
};

namespace detail
{

inline void generateQuad(std::vector<ArbitraryMeshVertex>& vertices, std::vector<unsigned int>& indices, 
    double size, const Vector4& colour)
{
    unsigned int indexOffset = static_cast<unsigned int>(vertices.size());

    vertices.push_back(ArbitraryMeshVertex({ -size,  size, 0 }, { 0,0,0 }, { 0,0 }, colour));
    vertices.push_back(ArbitraryMeshVertex({  size,  size, 0 }, { 0,0,0 }, { 0,0 }, colour));
    vertices.push_back(ArbitraryMeshVertex({  size, -size, 0 }, { 0,0,0 }, { 0,0 }, colour));
    vertices.push_back(ArbitraryMeshVertex({ -size, -size, 0 }, { 0,0,0 }, { 0,0 }, colour));

    indices.push_back(indexOffset + 0);
    indices.push_back(indexOffset + 1);
    indices.push_back(indexOffset + 1);
    indices.push_back(indexOffset + 2);
    indices.push_back(indexOffset + 2);
    indices.push_back(indexOffset + 3);
    indices.push_back(indexOffset + 3);
    indices.push_back(indexOffset + 0);
}

inline Vector4 toVector4(const Colour4b& colour)
{
    return
    {
        colour.r / 255.0,
        colour.g / 255.0,
        colour.b / 255.0,
        colour.a / 255.0,
    };
}

}

// Renders a fixed size point array as line strip
class RenderableLineStrip :
    public render::RenderableGeometry
{
protected:
    const Matrix4& _localToWorld;
    bool _needsUpdate;
    Vector4 _colour;

    std::vector<Vertex3f> _rawPoints;

public:
    void queueUpdate()
    {
        _needsUpdate = true;
    }

    void setColour(const Colour4b& colour)
    {
        _colour = detail::toVector4(colour);
        queueUpdate();
    }

    const std::vector<Vertex3f>& getRawPoints() const
    {
        return _rawPoints;
    }

protected:
    RenderableLineStrip(std::size_t numPoints, const Matrix4& localToWorld) :
        _localToWorld(localToWorld),
        _needsUpdate(true),
        _rawPoints(numPoints)
    {}

    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        std::vector<ArbitraryMeshVertex> vertices;
        std::vector<unsigned int> indices;

        unsigned int index = 0;

        for (const auto& vertex : _rawPoints)
        {
            vertices.push_back(ArbitraryMeshVertex(_localToWorld * vertex, { 0,0,0 }, { 0,0 }, _colour));

            if (index > 0)
            {
                indices.push_back(index - 1);
                indices.push_back(index);
            }

            ++index;
        }

        RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices, indices);
    }
};

template<typename RemapPolicy>
class RenderableSemiCircle :
    public RenderableLineStrip
{
public:
    RenderableSemiCircle(std::size_t segments, double radius, const Matrix4& localToWorld) :
        RenderableLineStrip((segments << 2) + 1, localToWorld)
    {
        draw_semicircle<RemapPolicy>(segments, radius, _rawPoints);
    }
};

template<typename RemapPolicy>
class RenderableCircle :
    public RenderableLineStrip
{
public:
    RenderableCircle(std::size_t segments, double radius, const Matrix4& localToWorld) :
        RenderableLineStrip(segments << 3, localToWorld)
    {
        draw_circle<RemapPolicy>(segments, radius, _rawPoints);
    }
};

}
