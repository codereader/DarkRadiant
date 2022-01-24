#pragma once

#include "math/Matrix4.h"
#include "render/RenderableTextBase.h"
#include "NameKey.h"

namespace entity
{

class EntityNode;

class RenderableEntityName :
    public render::RenderableTextBase
{
private:
    const EntityNode& _entity;
    const NameKey& _nameKey;
    Vector4 _colour;

public:
    RenderableEntityName(const EntityNode& entity, const NameKey& nameKey) :
        _entity(entity),
        _nameKey(nameKey)
    {}

    const Vector3& getWorldPosition() override;

    const std::string& getText() override;

    const Vector4& getColour() override;
};

}
