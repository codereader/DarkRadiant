#pragma once

#include "irender.h"
#include "irenderview.h"
#include "SceneRenderer.h"
#include "igeometrystore.h"

namespace render
{

class GLProgramFactory;
class LightInteractions;

class LightingModeRenderer final :
    public SceneRenderer
{
private:
    GLProgramFactory& _programFactory;

    IGeometryStore& _geometryStore;

    // The set of registered render lights
    const std::set<RendererLightPtr>& _lights;

    // The set of registered render entities
    const std::set<IRenderEntityPtr>& _entities;

    std::vector<IGeometryStore::Slot> _untransformedObjectsWithoutAlphaTest;

public:
    LightingModeRenderer(GLProgramFactory& programFactory, 
                         IGeometryStore& store,
                         const std::set<RendererLightPtr>& lights,
                         const std::set<IRenderEntityPtr>& entities) :
        _programFactory(programFactory),
        _geometryStore(store),
        _lights(lights),
        _entities(entities)
    {
        _untransformedObjectsWithoutAlphaTest.reserve(10000);
    }

    IRenderResult::Ptr render(RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t time) override;

private:
    std::size_t drawDepthFillPass(OpenGLState& current, RenderStateFlags globalFlagsMask,
        std::vector<LightInteractions>& interactionLists, const IRenderView& view, std::size_t renderTime);

    std::size_t drawNonInteractionPasses(OpenGLState& current, RenderStateFlags globalFlagsMask, 
        const IRenderView& view, std::size_t time);
};

}
