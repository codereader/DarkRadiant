#pragma once

#include "render/RenderableBox.h"

namespace entity
{

class EntityNode;

class RenderableEntityBox final :
    public render::RenderableBox
{
private:
    const IEntityNode& _entity;

public:
    RenderableEntityBox(const IEntityNode& entity, const AABB& bounds, const Vector3& worldPos);

protected:
    Vector4 getVertexColour() override;
};

}
