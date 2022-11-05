#include "RenderableArrow.h"

#include "EntityNode.h"

namespace entity
{

RenderableArrow::RenderableArrow(EntityNode& node) :
    _node(node),
    _needsUpdate(true)
{}

void RenderableArrow::queueUpdate()
{
    _needsUpdate = true;
}

void RenderableArrow::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    static Vector3 Up(0, 0, 1);
    
    // The starting point of the arrow is at the center of the entity's visible bounding box
    auto origin = _node.getWorldPosition() + _node.localAABB().getOrigin();

    const auto& direction = _node.getDirection();
    auto colour = _node.getRenderState() == scene::INode::RenderState::Active ?
        _node.getEntityColour() : INACTIVE_ENTITY_COLOUR;

    Vector3 left(-direction.y(), direction.x(), 0);

    Vector3 endpoint(origin + direction * 32.0);

    Vector3 tip1(endpoint + direction * (-8.0) + Up * (-4.0));
    Vector3 tip2(tip1 + Up * 8.0);
    Vector3 tip3(endpoint + direction * (-8.0) + left * (-4.0));
    Vector3 tip4(tip3 + left * 8.0);

    std::vector<render::RenderVertex> vertices
    {
        render::RenderVertex(origin, {1,0,0}, {0,0}, colour),
        render::RenderVertex(endpoint, {1,0,0}, {0,0}, colour),
        render::RenderVertex(tip1, {1,0,0}, {0,0}, colour),
        render::RenderVertex(tip2, {1,0,0}, {0,0}, colour),
        render::RenderVertex(tip3, {1,0,0}, {0,0}, colour),
        render::RenderVertex(tip4, {1,0,0}, {0,0}, colour),
    };

    // Indices are always the same, therefore constant
    static const std::vector<unsigned int> Indices
    {
        0, 1, // origin to endpoint
        1, 2, // endpoint to tip1
        1, 3, // endpoint to tip2
        1, 4, // endpoint to tip3
        1, 5, // endpoint to tip4
        2, 4, // tip1 to tip3
        4, 3, // tip3 to tip2
        3, 5, // tip2 to tip4
        5, 2, // tip4 to tip1
    };

    updateGeometryWithData(render::GeometryType::Lines, vertices, Indices);
}

}
