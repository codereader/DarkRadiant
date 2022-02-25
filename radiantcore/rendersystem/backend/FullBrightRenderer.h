#pragma once

#include "SceneRenderer.h"
#include "OpenGLStateManager.h"

namespace render
{

class FullBrightRenderer :
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

    void render(RenderStateFlags globalstate, const IRenderView& view, std::size_t time);
};

}
