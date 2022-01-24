#include "RenderableEntityName.h"
#include "EntityNode.h"

namespace entity
{

const Vector3& RenderableEntityName::getWorldPosition()
{
    return _entity.getWorldPosition();
}

const std::string& RenderableEntityName::getText()
{
    return _nameKey.getName();
}

const Vector4& RenderableEntityName::getColour()
{
    // Keep the local copy up to date by querying the owning entity every time
    _colour = _entity.getEntityColour();

    return _colour;
}

}
