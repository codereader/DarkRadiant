#pragma once

#include <map>
#include <vector>
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

    // A flat list of surfaces
    using SurfaceList = std::vector<std::reference_wrapper<IRenderableSurface>>;

    // All surfaces, grouped by material
    using SurfacesByMaterial = std::map<OpenGLShader*, SurfaceList>;

    // SurfaceLists, grouped by entity
    std::map<IRenderEntity*, SurfacesByMaterial> _surfacesByEntity;

    std::size_t _drawCalls;

public:
    LightInteractions(RendererLight& light) :
        _light(light),
        _drawCalls(0)
    {}

    std::size_t getDrawCalls() const
    {
        return _drawCalls;
    }

    void addSurface(IRenderableSurface& surface, IRenderEntity& entity, OpenGLShader& shader);

    void fillDepthBuffer(OpenGLState& current, RenderStateFlags globalFlagsMask, 
        const IRenderView& view, std::size_t renderTime);

    void render(OpenGLState& state, RenderStateFlags globalFlagsMask, 
        const IRenderView& view, std::size_t renderTime);
};

}
