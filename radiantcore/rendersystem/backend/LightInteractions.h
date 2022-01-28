#pragma once

#include <map>
#include <vector>
#include <set>
#include "irender.h"
#include "isurfacerenderer.h"
#include "irenderview.h"

namespace render
{

class OpenGLShader;

/**
 * Defines interactions between a light and one or more entity surfaces
 * It only lives through the course of a single render pass, therefore direct
 * references without ref-counting are used.
 * 
 * Surfaces are grouped by entity, then by shader.
 */
class LightInteractions
{
private:
    RendererLight& _light;
    AABB _lightBounds;

    // A flat list of surfaces
    using SurfaceList = std::vector<std::reference_wrapper<IRenderableSurface>>;

    // All surfaces, grouped by material
    using SurfacesByMaterial = std::map<OpenGLShader*, SurfaceList>;

    // SurfaceLists, grouped by entity
    std::map<IRenderEntity*, SurfacesByMaterial> _surfacesByEntity;

    std::size_t _drawCalls;
    std::size_t _surfaceCount;

public:
    LightInteractions(RendererLight& light) :
        _light(light),
        _lightBounds(light.lightAABB()),
        _drawCalls(0),
        _surfaceCount(0)
    {}

    std::size_t getDrawCalls() const
    {
        return _drawCalls;
    }

    std::size_t getSurfaceCount() const
    {
        return _surfaceCount;
    }

    std::size_t getEntityCount() const
    {
        return _surfacesByEntity.size();
    }

    void addSurface(IRenderableSurface& surface, IRenderEntity& entity, OpenGLShader& shader);

    bool isInView(const IRenderView& view);

    void collectSurfaces(const std::set<IRenderEntityPtr>& entities);

    void fillDepthBuffer(OpenGLState& current, RenderStateFlags globalFlagsMask, 
        const IRenderView& view, std::size_t renderTime);

    void render(OpenGLState& state, RenderStateFlags globalFlagsMask, 
        const IRenderView& view, std::size_t renderTime);
};

}
