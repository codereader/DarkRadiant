#pragma once

#include "render/RenderableBox.h"

class IEntityNode;

namespace entity
{

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
