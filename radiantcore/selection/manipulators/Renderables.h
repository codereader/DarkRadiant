#pragma once

#include "math/AABB.h"
#include "render/RenderableGeometry.h"
#include "render/RenderableBox.h"
#include "render.h"

namespace selection
{

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
        _colour(1, 1, 1, 1)
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

        std::vector<render::RenderVertex> vertices;
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

        updateGeometryWithData(render::GeometryType::Points, vertices, indices);
    }
};

namespace detail
{

inline void generateQuad(std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices,
    double size, const Vector4& colour)
{
    unsigned int indexOffset = static_cast<unsigned int>(vertices.size());

    vertices.push_back(MeshVertex({ -size,  size, 0 }, { 0,0,0 }, { 0,0 }, colour));
    vertices.push_back(MeshVertex({  size,  size, 0 }, { 0,0,0 }, { 0,0 }, colour));
    vertices.push_back(MeshVertex({  size, -size, 0 }, { 0,0,0 }, { 0,0 }, colour));
    vertices.push_back(MeshVertex({ -size, -size, 0 }, { 0,0,0 }, { 0,0 }, colour));

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

    std::vector<Vertex3> _rawPoints;

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

    const std::vector<Vertex3>& getRawPoints() const
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

        std::vector<render::RenderVertex> vertices;
        std::vector<unsigned int> indices;

        unsigned int index = 0;

        for (const auto& vertex : _rawPoints)
        {
            vertices.push_back(render::RenderVertex(_localToWorld * vertex, { 0,0,0 }, { 0,0 }, _colour));

            if (index > 0)
            {
                indices.push_back(index - 1);
                indices.push_back(index);
            }

            ++index;
        }

        updateGeometryWithData(render::GeometryType::Lines, vertices, indices);
    }
};

/// Line strip in the shape of a semicircle
class RenderableSemiCircle :
    public RenderableLineStrip
{
public:

    /**
     * @brief Construct a new RenderableSemiCircle object
     *
     * @param segments
     * Number of linear segments to divide the semicircle into.
     *
     * @param radius
     * Radius of the semicircle in world units.
     *
     * @param localToWorld
     * Local space transformation matrix.
     *
     * @param axisRemap
     * Optional matrix to reassign axes, used to orient the semicircle in 3D space.
     */
    RenderableSemiCircle(std::size_t segments, double radius, const Matrix4& localToWorld,
                         const Matrix4& axisRemap)
    : RenderableLineStrip((segments << 2) + 1, localToWorld)
    {
        const double increment = math::PI / double(segments << 2);

        std::size_t count = 0;
        double x = radius;
        double y = 0;
        _rawPoints[segments << 2] = axisRemap * Vertex3(-radius, 0, 0);

        while (count < segments) {
            auto& i = _rawPoints[count];
            auto& j = _rawPoints[(segments << 1) - (count + 1)];

            auto& k = _rawPoints[count + (segments << 1)];
            auto& l = _rawPoints[(segments << 1) - (count + 1) + (segments << 1)];

            i = axisRemap * Vertex3(x, -y, 0);
            k = axisRemap * Vertex3(-y, -x, 0);

            ++count;

            const double theta = increment * count;
            x = radius * cos(theta);
            y = radius * sin(theta);

            j = axisRemap * Vertex3(y, -x, 0);
            l = axisRemap * Vertex3(-x, -y, 0);
        }
    }
};

/// Line strip in the shape of a full circle
class RenderableCircle: public RenderableLineStrip
{
public:
    RenderableCircle(std::size_t segments, double radius, const Matrix4& localToWorld) :
        RenderableLineStrip(segments << 3, localToWorld)
    {
        draw_circle<RemapXYZ>(segments, radius, _rawPoints);
    }
};

class RenderableArrowLine :
    public RenderableLineStrip
{
public:
    RenderableArrowLine(const Vector3& direction, const Matrix4& localToWorld) :
        RenderableLineStrip(2, localToWorld)
    {
        _rawPoints[0] = Vector3(0, 0, 0);
        _rawPoints[1] = direction;
    }
};

class RenderableQuad :
    public RenderableLineStrip
{
public:
    RenderableQuad(double edgeLength, const Matrix4& localToWorld) :
        RenderableLineStrip(5, localToWorld)
    {
        _rawPoints[0] = Vector3(edgeLength, edgeLength, 0);
        _rawPoints[1] = Vector3(edgeLength, -edgeLength, 0);
        _rawPoints[2] = Vector3(-edgeLength, -edgeLength, 0);
        _rawPoints[3] = Vector3(-edgeLength, edgeLength, 0);
        _rawPoints[4] = _rawPoints[0];
    }
};

// Renders a few flat-shaded triangles as arrow head, offset by a given amount
class RenderableArrowHead :
    public render::RenderableGeometry
{
protected:
    Vector3 _offset;
    const Vector3& _screenAxis;
    double _width;
    double _height;
    const Matrix4& _localToWorld;
    bool _needsUpdate;
    Vector4 _colour;

    std::vector<Vertex3> _rawPoints;

public:
    RenderableArrowHead(const Vector3& offset, const Vector3& screenAxis, double width, double height, const Matrix4& localToWorld) :
        _offset(offset),
        _screenAxis(screenAxis),
        _width(width),
        _height(height),
        _localToWorld(localToWorld),
        _needsUpdate(true),
        _rawPoints(3)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

    void setColour(const Colour4b& colour)
    {
        _colour = detail::toVector4(colour);
        queueUpdate();
    }

    const std::vector<Vertex3>& getRawPoints() const
    {
        return _rawPoints;
    }

protected:
    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        std::vector<render::RenderVertex> vertices;
        std::vector<unsigned int> indices;

        auto direction = _offset.getNormalised();
        auto sideWays = _offset.cross(_screenAxis).getNormalised();

        _rawPoints[0] = _offset; // tip
        _rawPoints[1] = _offset - direction * _height + sideWays * _width;
        _rawPoints[2] = _offset - direction * _height - sideWays * _width;

        unsigned int index = 0;

        for (const auto& vertex : _rawPoints)
        {
            vertices.push_back(render::RenderVertex(_localToWorld * vertex, _screenAxis, { 0,0 }, _colour));
            indices.push_back(index++);
        }

        updateGeometryWithData(render::GeometryType::Triangles, vertices, indices);
    }
};

class RenderablePoint :
    public render::RenderableGeometry
{
protected:
    const Vertex3& _point;
    const Matrix4& _localToWorld;
    bool _needsUpdate;
    Vector4 _colour;

public:
    RenderablePoint(const Vertex3& point, const Matrix4& localToWorld) :
        _point(point),
        _localToWorld(localToWorld),
        _needsUpdate(true)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

    void setColour(const Colour4b& colour)
    {
        _colour = detail::toVector4(colour);
        queueUpdate();
    }

protected:
    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        std::vector<render::RenderVertex> vertices;
        std::vector<unsigned int> indices;

        vertices.push_back(render::RenderVertex(_localToWorld * _point, { 0,0,0 }, { 0,0 }, _colour));
        indices.push_back(0);

        updateGeometryWithData(render::GeometryType::Points, vertices, indices);
    }
};

}
