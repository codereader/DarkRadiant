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
public:
    SceneRenderer(const SceneRenderer& other) = delete;
    SceneRenderer& operator=(const SceneRenderer& other) = delete;

protected:
    SceneRenderer()
    {}

    virtual ~SceneRenderer()
    {}

    // Set the projection and modelview matrices
    void setupViewMatrices(const IRenderView& view);

    void beginRendering(OpenGLState& state);
    void finishRendering();
};

}
