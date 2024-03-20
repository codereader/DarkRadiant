#pragma once

#include "render/RenderableBox.h"

class EntityNode;

namespace entity
{

class RenderableEntityBox final :
    public render::RenderableBox
{
private:
    const EntityNode& _entity;

public:
    RenderableEntityBox(const EntityNode& entity, const AABB& bounds, const Vector3& worldPos);

protected:
    Vector4 getVertexColour() override;
};

}
