#pragma once

#include "SceneRenderer.h"
#include "OpenGLStateManager.h"

namespace render
{

class FullBrightRenderer :
    public SceneRenderer
{
private:
    const OpenGLStates& _sortedStates;

public:
    FullBrightRenderer(const OpenGLStates& sortedStates) :
        _sortedStates(sortedStates)
    {}

    void render(RenderViewType renderViewType, RenderStateFlags globalstate, 
        const IRenderView& view, std::size_t time);
};

}
