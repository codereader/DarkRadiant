#include "Renderables.h"

#include "LightNode.h"
#include "../EntitySettings.h"

namespace entity
{

namespace
{

inline void applyTransform(std::vector<ArbitraryMeshVertex>& vertices, const Matrix4& transform)
{
    for (auto& vertex : vertices)
    {
        vertex.vertex = transform * vertex.vertex;
    }
}

}

void RenderableLightOctagon::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    // Generate the indexed vertex data
    static Vector3 Origin(0, 0, 0);
    static Vector3 Extents(8, 8, 8);

    // Calculate the light vertices of this bounding box and store them into <points>
    Vector3 max(Origin + Extents);
    Vector3 min(Origin - Extents);
    Vector3 mid(Origin);

    auto colour = _light.getEntityColour();

    // top, bottom, tleft, tright, bright, bleft
    std::vector<ArbitraryMeshVertex> vertices
    {
        ArbitraryMeshVertex({ mid[0], mid[1], max[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ mid[0], mid[1], min[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ min[0], max[1], mid[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ max[0], max[1], mid[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ max[0], min[1], mid[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ min[0], min[1], mid[2] }, {1,0,0}, {0,0}, colour),
    };

    // Orient the points using the transform
    applyTransform(vertices, _light.localToWorld());

    // Indices are always the same, therefore constant
    static const std::vector<unsigned int> Indices
    {
        0, 2, 3,
        0, 3, 4,
        0, 4, 5,
        0, 5, 2,
        1, 2, 5,
        1, 5, 4,
        1, 4, 3,
        1, 3, 2
    };

    RenderableGeometry::updateGeometry(render::GeometryType::Triangles, vertices, Indices);
}

void RenderableLightVolume::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    if (_light.isProjected())
    {
        updateProjectedLightVolume();
    }
    else
    {
        updatePointLightVolume();
    }
}

void RenderableLightVolume::updatePointLightVolume()
{
    static Vector3 Origin(0, 0, 0);

    const auto& radius = _light.getLightRadius();

    // Calculate the corner vertices of this bounding box, plus the mid-point
    Vector3 max(Origin + radius);
    Vector3 min(Origin - radius);

    auto colour = _light.getEntityColour();

    // Load the 8 corner points
    std::vector<ArbitraryMeshVertex> vertices
    {
        ArbitraryMeshVertex({ min[0], min[1], min[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ max[0], min[1], min[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ max[0], max[1], min[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ min[0], max[1], min[2] }, {1,0,0}, {0,0}, colour),

        ArbitraryMeshVertex({ min[0], min[1], max[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ max[0], min[1], max[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ max[0], max[1], max[2] }, {1,0,0}, {0,0}, colour),
        ArbitraryMeshVertex({ min[0], max[1], max[2] }, {1,0,0}, {0,0}, colour),
    };

    // Orient the points using the transform
    applyTransform(vertices, _light.localToWorld());

    static const std::vector<unsigned int> Indices
    {
        0, 1, // bottom rectangle
        1, 2, //
        2, 3, //
        3, 0, //

        4, 5, // top rectangle
        5, 6, //
        6, 7, //
        7, 4, //

        0, 4, // vertical edges
        1, 5, //
        2, 6, //
        3, 7, //

        0, 6, // diagonals
        1, 7, //
        2, 4, //
        3, 5, //
    };

    RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices, Indices);
}

void RenderableLightVolume::updateProjectedLightVolume()
{
    const auto& frustum = _light.getLightFrustum();

    // greebo: These four define the base area and are always needed to draw the light
    auto backUpperLeft = frustum.getCornerPoint(Frustum::BACK, Frustum::TOP_LEFT);
    auto backLowerLeft = frustum.getCornerPoint(Frustum::BACK, Frustum::BOTTOM_LEFT);
    auto backUpperRight = frustum.getCornerPoint(Frustum::BACK, Frustum::TOP_RIGHT);
    auto backLowerRight = frustum.getCornerPoint(Frustum::BACK, Frustum::BOTTOM_RIGHT);

    const auto& lightStart = _light.getLightStart();

    auto colour = _light.getEntityColour();

    if (lightStart != Vector3(0, 0, 0))
    {
        // Calculate the vertices defining the top area
        auto frontUpperLeft = frustum.getCornerPoint(Frustum::FRONT, Frustum::TOP_LEFT);
        auto frontLowerLeft = frustum.getCornerPoint(Frustum::FRONT, Frustum::BOTTOM_LEFT);
        auto frontUpperRight = frustum.getCornerPoint(Frustum::FRONT, Frustum::TOP_RIGHT);
        auto frontLowerRight = frustum.getCornerPoint(Frustum::FRONT, Frustum::BOTTOM_RIGHT);

        std::vector<ArbitraryMeshVertex> vertices
        {
            ArbitraryMeshVertex(frontUpperLeft, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(frontLowerLeft, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(frontLowerRight, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(frontUpperRight, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(backUpperLeft, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(backLowerLeft, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(backLowerRight, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(backUpperRight, {1,0,0}, {0,0}, colour),
        };

        // Orient the points using the transform
        applyTransform(vertices, _light.localToWorld());

        static const std::vector<unsigned int> Indices
        {
            0, 4, // top up right to bottom up right
            1, 5, // top down right to bottom down right
            2, 6, // top down left to bottom down left
            3, 7, // top up left to bottom up left

            0, 1, // top up right to top down right
            1, 2, // top down right to top down left
            2, 3, // top down left to top up left
            3, 0, // top up left to top up right

            4, 5, // bottom up right to bottom down right
            5, 6, // bottom down right to bottom down left
            6, 7, // bottom down left to bottom up left
            7, 4, // bottom up left to bottom up right
        };

        RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices, Indices);
    }
    else
    {
        // no light_start, just use the top vertex (doesn't need to be mirrored)
        auto top = Plane3::intersect(frustum.left, frustum.right, frustum.top);

        std::vector<ArbitraryMeshVertex> vertices
        {
            ArbitraryMeshVertex(top, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(backUpperLeft, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(backLowerLeft, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(backLowerRight, {1,0,0}, {0,0}, colour),
            ArbitraryMeshVertex(backUpperRight, {1,0,0}, {0,0}, colour),
        };

        // Orient the points using the transform
        applyTransform(vertices, _light.localToWorld());

        static const std::vector<unsigned int> Indices
        {
          0, 1, // top to first
          0, 2, // top to second
          0, 3, // top to third
          0, 4, // top to fourth
          1, 2, // first to second
          2, 3, // second to third
          3, 4, // third to fourth
          4, 1, // fourth to first
        };

        RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices, Indices);
    }
}

namespace detail
{

inline void addVertex(std::vector<ArbitraryMeshVertex>& vertices, std::vector<unsigned int>& indices,
    const Vector3& vertex, const Vector4& colour)
{
    indices.push_back(static_cast<unsigned int>(vertices.size()));
    vertices.push_back(ArbitraryMeshVertex(vertex, { 0,0,0 }, { 0,0 }, colour));
}

}

void RenderableLightVertices::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    std::vector<ArbitraryMeshVertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(LightVertexInstanceSet::NumVertices);
    indices.reserve(LightVertexInstanceSet::NumVertices);

    auto& settings = *EntitySettings::InstancePtr();
    const auto& colourVertexSelected = settings.getLightVertexColour(LightEditVertexType::Selected);
    const auto& colourVertexDeselected = settings.getLightVertexColour(LightEditVertexType::Deselected);
    const auto& colourVertexInactive = settings.getLightVertexColour(LightEditVertexType::Inactive);
    const auto& colourStartEndSelected = settings.getLightVertexColour(LightEditVertexType::StartEndSelected);
    const auto& colourStartEndDeselected = settings.getLightVertexColour(LightEditVertexType::StartEndDeselected);

    // Local colour evaluation lambdas
    auto getRegularVertexColour = [&](const VertexInstance& instance)->const Vector3&
    {
        return _mode != selection::ComponentSelectionMode::Vertex ? colourVertexInactive :
            instance.isSelected() ? colourVertexSelected : colourVertexDeselected;
    };

    auto getStartEndVertexColour = [&](const VertexInstance& instance)->const Vector3&
    {
        return _mode != selection::ComponentSelectionMode::Vertex ? colourVertexInactive :
            instance.isSelected() ? colourStartEndSelected : colourStartEndDeselected;
    };

    if (_light.isProjected())
    {
        detail::addVertex(vertices, indices, _instances.target.getVertex(), getRegularVertexColour(_instances.target));
        detail::addVertex(vertices, indices, _instances.right.getVertex(), getRegularVertexColour(_instances.right));
        detail::addVertex(vertices, indices, _instances.up.getVertex(), getRegularVertexColour(_instances.up));

        if (_useFlags.start)
        {
            detail::addVertex(vertices, indices, _instances.start.getVertex(), getStartEndVertexColour(_instances.start));
        }

        if (_useFlags.end)
        {
            detail::addVertex(vertices, indices, _instances.end.getVertex(), getStartEndVertexColour(_instances.end));
        }
    }
    else
    {
        // Not a projected light, include just the light centre vertex
        detail::addVertex(vertices, indices, _instances.center.getVertex(), getRegularVertexColour(_instances.center));
    }

    // Apply the local2world transform to all the vertices
    const auto& local2World = _light.localToWorld();

    for (auto& vertex : vertices)
    {
        vertex.vertex = local2World * vertex.vertex;
    }

    RenderableGeometry::updateGeometry(render::GeometryType::Points, vertices, indices);
}

} // namespace
