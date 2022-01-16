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
    return _entity.getEntityColour();
}

}
