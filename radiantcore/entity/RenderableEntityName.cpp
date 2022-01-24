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
    static Vector4 colour(1, 1, 1, 1);
    return colour;
}

}
