#pragma once

#include "irender.h"
#include "irenderview.h"
#include "SceneRenderer.h"
#include "igeometrystore.h"

namespace render
{

class LightingModeRenderer final :
    public SceneRenderer
{
private:
    IGeometryStore& _geometryStore;

    // The set of registered render lights
    const std::set<RendererLightPtr>& _lights;

    // The set of registered render entities
    const std::set<IRenderEntityPtr>& _entities;

public:
    LightingModeRenderer(IGeometryStore& store, 
                         const std::set<RendererLightPtr>& lights,
                         const std::set<IRenderEntityPtr>& entities) :
        _geometryStore(store),
        _lights(lights),
        _entities(entities)
    {}

    IRenderResult::Ptr render(RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t time) override;
};

}
