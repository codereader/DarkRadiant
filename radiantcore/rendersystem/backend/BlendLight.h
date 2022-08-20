#pragma once

#include "irender.h"

namespace render
{

class IGeometryStore;
class IObjectRenderer;

/**
 * BlendLights are non-shadowcasting lights performing a simple blend operation
 * on any surfaces they intersect with.
 * The type of blend operation is defined in the stages of the light material.
 *
 * Instances only live through the course of a single render pass, therefore direct
 * references without ref-counting are used.
 */
class BlendLight
{
private:
    RendererLight& _light;
    IGeometryStore& _store;
    IObjectRenderer& _objectRenderer;
    AABB _lightBounds;

    using ObjectList = std::vector<std::reference_wrapper<IRenderableObject>>;
    ObjectList _objects;

    std::size_t _objectCount;

public:
    BlendLight(RendererLight& light, IGeometryStore& store, IObjectRenderer& objectRenderer);
    BlendLight(BlendLight&& other) = default;

    bool isInView(const IRenderView& view);
    void collectSurfaces(const IRenderView& view, const std::set<IRenderEntityPtr>& entities);

    std::size_t getObjectCount() const
    {
        return _objectCount;
    }
};

}
