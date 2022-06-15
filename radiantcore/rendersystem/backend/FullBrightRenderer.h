#pragma once

#include "iobjectrenderer.h"
#include "SceneRenderer.h"
#include "OpenGLStateManager.h"

namespace render
{

class FullBrightRenderer final :
    public SceneRenderer
{
private:
    const OpenGLStates& _sortedStates;
    IGeometryStore& _geometryStore;
    IObjectRenderer& _objectRenderer;

public:
    FullBrightRenderer(RenderViewType renderViewType, const OpenGLStates& sortedStates, 
                       IGeometryStore& geometryStore, IObjectRenderer& objectRenderer) :
        SceneRenderer(renderViewType),
        _sortedStates(sortedStates),
        _geometryStore(geometryStore),
        _objectRenderer(objectRenderer)
    {}

    IRenderResult::Ptr render(RenderStateFlags globalstate, const IRenderView& view, std::size_t time) override;
};

}
