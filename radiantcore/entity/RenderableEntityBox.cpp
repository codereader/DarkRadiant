#include "RenderableEntityBox.h"

#include "EntityNode.h"

namespace entity
{

RenderableEntityBox::RenderableEntityBox(const IEntityNode& entity, const AABB& bounds, const Vector3& worldPos) :
    RenderableBox(bounds, worldPos),
    _entity(entity)
{}

Vector4 RenderableEntityBox::getVertexColour()
{
    // Return white if this entity is inactive, this leaves the shader colour alive
    return _entity.getRenderState() == scene::INode::RenderState::Active ? _entity.getEntityColour() : INACTIVE_ENTITY_COLOUR;
}

}
