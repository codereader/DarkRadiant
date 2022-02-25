#pragma once

class OpenGLState;

namespace render
{

class IRenderView;

/**
 * Common base class for FullBright and LightingMode renderers.
 * Houses shared openGL state management code.
 */
class SceneRenderer
{
protected:
    SceneRenderer()
    {}

    SceneRenderer(const SceneRenderer& other) = delete;
    SceneRenderer& operator=(const SceneRenderer& other) = delete;

public:
    virtual ~SceneRenderer()
    {}

    virtual IRenderResult::Ptr render(RenderStateFlags globalstate, const IRenderView& view, std::size_t time) = 0;

protected:
    // Set the projection and modelview matrices
    void setupViewMatrices(const IRenderView& view);

    void setupState(OpenGLState& state);
    void cleanupState();
};

}
