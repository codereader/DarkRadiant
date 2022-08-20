#pragma once

#include "irender.h"

namespace render
{

class IGeometryStore;
class IObjectRenderer;

class BlendLight
{
private:
    RendererLight& _light;
    IGeometryStore& _store;
    IObjectRenderer& _objectRenderer;
    AABB _lightBounds;

public:
    BlendLight(RendererLight& light, IGeometryStore& store, IObjectRenderer& objectRenderer);
    BlendLight(BlendLight&& other) = default;
};

}
