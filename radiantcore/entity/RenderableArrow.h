#pragma once

#include "render/RenderableGeometry.h"

namespace entity
{

class EntityNode;

class RenderableArrow :
    public render::RenderableGeometry
{
private:
    const EntityNode& _node;
    bool _needsUpdate;

public:
    RenderableArrow(EntityNode& node);

    void queueUpdate();

    virtual void updateGeometry() override;
};

}
