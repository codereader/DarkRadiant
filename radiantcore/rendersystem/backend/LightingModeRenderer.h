#pragma once

#include "irender.h"
#include "irenderview.h"
#include "SceneRenderer.h"
#include "igeometrystore.h"
#include "iobjectrenderer.h"
#include "FrameBuffer.h"
#include "render/Rectangle.h"
#include "glprogram/ShadowMapProgram.h"
#include "InteractingLight.h"
#include "registry/CachedKey.h"

namespace render
{

class GLProgramFactory;
class LightingModeRenderResult;

class LightingModeRenderer final :
    public SceneRenderer
{
private:
    GLProgramFactory& _programFactory;

    IGeometryStore& _geometryStore;
    IObjectRenderer& _objectRenderer;

    // The set of registered render lights
    const std::set<RendererLightPtr>& _lights;

    // The set of registered render entities
    const std::set<IRenderEntityPtr>& _entities;

    std::vector<IGeometryStore::Slot> _untransformedObjectsWithoutAlphaTest;

    FrameBuffer::Ptr _shadowMapFbo;
    std::vector<Rectangle> _shadowMapAtlas;
    ShadowMapProgram* _shadowMapProgram;

    constexpr static std::size_t MaxShadowCastingLights = 6;

    registry::CachedKey<bool> _shadowMappingEnabled;

    // Data that is valid during a single render pass only

    std::vector<InteractingLight> _interactingLights;
    std::vector<InteractingLight*> _nearestShadowLights;
    std::shared_ptr<LightingModeRenderResult> _result;

public:
    LightingModeRenderer(GLProgramFactory& programFactory,
        IGeometryStore& store,
        IObjectRenderer& objectRenderer,
        const std::set<RendererLightPtr>& lights,
        const std::set<IRenderEntityPtr>& entities);

    IRenderResult::Ptr render(RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t time) override;

private:
    void determineInteractingLight(const IRenderView& view);

    void drawInteractingLights(OpenGLState& current, RenderStateFlags globalFlagsMask,
        const IRenderView& view, std::size_t renderTime);

    void drawDepthFillPass(OpenGLState& current, RenderStateFlags globalFlagsMask,
        const IRenderView& view, std::size_t renderTime);

    void drawNonInteractionPasses(OpenGLState& current, RenderStateFlags globalFlagsMask, 
        const IRenderView& view, std::size_t time);

    void drawShadowMaps(OpenGLState& current, std::size_t renderTime);

    void ensureShadowMapSetup();

    void addToShadowLights(InteractingLight& light, const Vector3& viewer);
};

}
