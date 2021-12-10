#include "RenderableEntityBox.h"

#include "EntityNode.h"

namespace entity
{

namespace
{

inline std::vector<ArbitraryMeshVertex> getFillBoxVertices(const Vector3& min, const Vector3& max)
{
    // Load the 6 times 4 = 24 corner points, each with the correct face normal
    return
    {
        // Bottom quad
        ArbitraryMeshVertex({ min[0], min[1], min[2] }, {0,0,-1}, {0,0}),
        ArbitraryMeshVertex({ max[0], min[1], min[2] }, {0,0,-1}, {0,0}),
        ArbitraryMeshVertex({ max[0], max[1], min[2] }, {0,0,-1}, {0,0}),
        ArbitraryMeshVertex({ min[0], max[1], min[2] }, {0,0,-1}, {0,0}),

        // Top quad
        ArbitraryMeshVertex({ min[0], min[1], max[2] }, {0,0,+1}, {0,0}),
        ArbitraryMeshVertex({ max[0], min[1], max[2] }, {0,0,+1}, {0,0}),
        ArbitraryMeshVertex({ max[0], max[1], max[2] }, {0,0,+1}, {0,0}),
        ArbitraryMeshVertex({ min[0], max[1], max[2] }, {0,0,+1}, {0,0}),

        // Front quad
        ArbitraryMeshVertex({ min[0], min[1], min[2] }, {0,-1,0}, {0,0}),
        ArbitraryMeshVertex({ max[0], min[1], min[2] }, {0,-1,0}, {0,0}),
        ArbitraryMeshVertex({ max[0], min[1], max[2] }, {0,-1,0}, {0,0}),
        ArbitraryMeshVertex({ min[0], min[1], max[2] }, {0,-1,0}, {0,0}),

        // Back quad
        ArbitraryMeshVertex({ max[0], max[1], min[2] }, {0,+1,0}, {0,0}),
        ArbitraryMeshVertex({ min[0], max[1], min[2] }, {0,+1,0}, {0,0}),
        ArbitraryMeshVertex({ min[0], max[1], max[2] }, {0,+1,0}, {0,0}),
        ArbitraryMeshVertex({ max[0], max[1], max[2] }, {0,+1,0}, {0,0}),

        // Right quad
        ArbitraryMeshVertex({ max[0], min[1], min[2] }, {+1,0,0}, {0,0}),
        ArbitraryMeshVertex({ max[0], max[1], min[2] }, {+1,0,0}, {0,0}),
        ArbitraryMeshVertex({ max[0], max[1], max[2] }, {+1,0,0}, {0,0}),
        ArbitraryMeshVertex({ min[0], max[1], max[2] }, {+1,0,0}, {0,0}),

        // Left quad
        ArbitraryMeshVertex({ min[0], max[1], min[2] }, {-1,0,0}, {0,0}),
        ArbitraryMeshVertex({ min[0], min[1], min[2] }, {-1,0,0}, {0,0}),
        ArbitraryMeshVertex({ min[0], min[1], max[2] }, {-1,0,0}, {0,0}),
        ArbitraryMeshVertex({ min[0], max[1], max[2] }, {-1,0,0}, {0,0}),
    };
}

inline std::vector<ArbitraryMeshVertex> getWireframeBoxVertices(const Vector3& min, const Vector3& max)
{
    // Load the 8 corner points
    return
    {
        // Bottom quad
        ArbitraryMeshVertex({ min[0], min[1], min[2] }, {0,0,1}, {0,0}),
        ArbitraryMeshVertex({ max[0], min[1], min[2] }, {0,0,1}, {0,0}),
        ArbitraryMeshVertex({ max[0], max[1], min[2] }, {0,0,1}, {0,0}),
        ArbitraryMeshVertex({ min[0], max[1], min[2] }, {0,0,1}, {0,0}),

        // Top quad
        ArbitraryMeshVertex({ min[0], min[1], max[2] }, {0,0,1}, {0,0}),
        ArbitraryMeshVertex({ max[0], min[1], max[2] }, {0,0,1}, {0,0}),
        ArbitraryMeshVertex({ max[0], max[1], max[2] }, {0,0,1}, {0,0}),
        ArbitraryMeshVertex({ min[0], max[1], max[2] }, {0,0,1}, {0,0}),
    };
}

// Indices drawing a hollow box outline, corresponding to the order in getWireframeBoxVertices()
static const std::vector<unsigned int> WireframeBoxIndices
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
};

// Indices drawing a hollow box outline, corresponding to the order in getFillBoxVertices()
static const std::vector<unsigned int> FillBoxIndices
{
    3, 2, 1, 0, // bottom rectangle
    7, 6, 5, 4, // top rectangle

    4, 5, 1, 0, // sides
    5, 6, 2, 1,
    6, 7, 3, 2,
    7, 4, 0, 3,
};

}

RenderableEntityBox::RenderableEntityBox(EntityNode& node) :
    _node(node),
    _needsUpdate(true),
    _filledBox(true)
{}

void RenderableEntityBox::queueUpdate()
{
    _needsUpdate = true;
}

void RenderableEntityBox::setFillMode(bool fill)
{
    if (_filledBox != fill)
    {
        _filledBox = fill;
        clear();
        queueUpdate();
    }
}

void RenderableEntityBox::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    static Vector3 Origin(0, 0, 0);
    const auto& bounds = _node.localAABB();

    // Calculate the corner vertices of this bounding box, plus the mid-point
    Vector3 max(Origin + bounds.extents);
    Vector3 min(Origin - bounds.extents);

    auto vertices = _filledBox ? getFillBoxVertices(min, max) : getWireframeBoxVertices(min, max);

    // Move the points to their world position
    const auto& translation = _node.worldAABB().getOrigin();
    
    for (auto& vertex : vertices)
    {
        vertex.vertex += translation;
    }

    if (_filledBox)
    {
        RenderableGeometry::updateGeometry(render::GeometryType::Quads, vertices, FillBoxIndices);
    }
    else
    {
        RenderableGeometry::updateGeometry(render::GeometryType::Lines, vertices,  WireframeBoxIndices);
    }
}

}
