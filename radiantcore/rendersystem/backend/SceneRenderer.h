#pragma once

#include "irender.h"

namespace render
{

class OpenGLState;
class IRenderView;

/**
 * Common base class for FullBright and LightingMode renderers.
 * Houses shared openGL state management code.
 */
class SceneRenderer
{
protected:
    // The view type this renderer is designed for
    RenderViewType _renderViewType;

protected:
    SceneRenderer(RenderViewType viewType) :
        _renderViewType(viewType)
    {}

    SceneRenderer(const SceneRenderer& other) = delete;
    SceneRenderer& operator=(const SceneRenderer& other) = delete;

public:
    virtual ~SceneRenderer()
    {}

    virtual IRenderResult::Ptr render(RenderStateFlags globalstate, const IRenderView& view, std::size_t time) = 0;

    RenderViewType getViewType() const
    {
        return _renderViewType;
    }

protected:
    // Set the projection and modelview matrices
    void setupViewMatrices(const IRenderView& view);

    void setupState(OpenGLState& state);
    void cleanupState();
};

}
