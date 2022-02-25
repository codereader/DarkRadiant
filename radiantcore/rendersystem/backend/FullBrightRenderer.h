#pragma once

#include "SceneRenderer.h"
#include "OpenGLStateManager.h"

namespace render
{

class FullBrightRenderer final :
    public SceneRenderer
{
private:
    RenderViewType _renderViewType;
    const OpenGLStates& _sortedStates;

public:
    FullBrightRenderer(RenderViewType renderViewType, const OpenGLStates& sortedStates) :
        _renderViewType(renderViewType),
        _sortedStates(sortedStates)
    {}

    IRenderResult::Ptr render(RenderStateFlags globalstate, const IRenderView& view, std::size_t time) override;
};

}
