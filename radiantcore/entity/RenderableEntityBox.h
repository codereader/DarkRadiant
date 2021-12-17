#pragma once

#include "render/RenderableGeometry.h"

namespace entity
{

class EntityNode;

class RenderableEntityBox :
    public render::RenderableGeometry
{
private:
    const IEntityNode& _entity;
    const AABB& _bounds;
    const Vector3& _worldPos;
    bool _needsUpdate;
    bool _filledBox;

public:
    RenderableEntityBox(const IEntityNode& entity, const AABB& bounds, const Vector3& worldPos);

    void queueUpdate();
    void setFillMode(bool fill);

    virtual void updateGeometry() override;
};

}
