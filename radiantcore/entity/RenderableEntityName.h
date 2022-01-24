#pragma once

#include "math/Matrix4.h"
#include "render/RenderableTextBase.h"
#include "NameKey.h"

namespace entity
{

class RenderableEntityName :
    public render::RenderableTextBase
{
private:
    const NameKey& _nameKey;

    // The origin in world coordinates
    const Vector3& _entityOrigin;

public:
    RenderableEntityName(const NameKey& nameKey, const Vector3& entityOrigin) :
        _nameKey(nameKey),
        _entityOrigin(entityOrigin)
    {}

    const Vector3& getWorldPosition() override
    {
        return _entityOrigin;
    }

    const std::string& getText() override
    {
        return _nameKey.getName();
    }

    const Vector4& getColour() override
    {
        static Vector4 colour(1, 1, 1, 1);
        return colour;
    }
};

}
