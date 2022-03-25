#pragma once

#include "irender.h"
#include "irenderview.h"
#include "SceneRenderer.h"
#include "igeometrystore.h"
#include "FrameBuffer.h"
#include "render/Rectangle.h"
#include "glprogram/ShadowMapProgram.h"

namespace render
{

class GLProgramFactory;
class LightInteractions;
class LightingModeRenderResult;

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

    FrameBuffer::Ptr _shadowMapFbo;
    std::vector<Rectangle> _shadowMapAtlas;
    ShadowMapProgram* _shadowMapProgram;

    constexpr static std::size_t MaxShadowCastingLights = 6;

    // Data that is valid during a single render pass only

    std::vector<LightInteractions> _interactingLights;
    std::vector<LightInteractions*> _nearestShadowLights;
    std::shared_ptr<LightingModeRenderResult> _result;

public:
    LightingModeRenderer(GLProgramFactory& programFactory, 
                         IGeometryStore& store,
                         const std::set<RendererLightPtr>& lights,
                         const std::set<IRenderEntityPtr>& entities) :
        _programFactory(programFactory),
        _geometryStore(store),
        _lights(lights),
        _entities(entities),
        _shadowMapProgram(nullptr)
    {
        _untransformedObjectsWithoutAlphaTest.reserve(10000);
        _nearestShadowLights.reserve(MaxShadowCastingLights + 1);
    }

    IRenderResult::Ptr render(RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t time) override;

private:
    void determineLightInteractions(const IRenderView& view);

    void drawLightInteractions(OpenGLState& current, RenderStateFlags globalFlagsMask,
        const IRenderView& view, std::size_t renderTime);

    void drawDepthFillPass(OpenGLState& current, RenderStateFlags globalFlagsMask,
        const IRenderView& view, std::size_t renderTime);

    void drawNonInteractionPasses(OpenGLState& current, RenderStateFlags globalFlagsMask, 
        const IRenderView& view, std::size_t time);

    void drawShadowMaps(OpenGLState& current, std::size_t renderTime);

    void ensureShadowMapSetup();

    void addToShadowLights(LightInteractions& light, const Vector3& viewer);
};

}
