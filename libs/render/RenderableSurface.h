#pragma once

#include <vector>
#include "isurfacerenderer.h"
#include "irender.h"

namespace render
{

/**
 * Surface base type, handling vertex data updates in combination
 * with an ISurfaceRenderer instance.
 *
 * It implements the OpenGLRenderable interface which will instruct
 * the shader to render just the surface managed by this object.
 * This is used to render highlights (such as selection overlays).
 */
class RenderableSurface :
    public IRenderableSurface,
    public OpenGLRenderable
{
private:
    ShaderPtr _shader;
    ISurfaceRenderer::Slot _surfaceSlot;

    std::size_t _lastVertexSize; // To detect size changes when updating surfaces
    std::size_t _lastIndexSize; // To detect size changes when updating surfaces

protected:
    RenderableSurface() :
        _surfaceSlot(ISurfaceRenderer::InvalidSlot),
        _lastVertexSize(0),
        _lastIndexSize(0)
    {}

public:
    // Noncopyable
    RenderableSurface(const RenderableSurface& other) = delete;
    RenderableSurface& operator=(const RenderableSurface& other) = delete;

    virtual ~RenderableSurface()
    {
        clear();
    }

    // (Non-virtual) update method handling any possible shader change
    // The surface is withdrawn from the given shader if it turns out
    // to be different from the last update.
    void update(const ShaderPtr& shader)
    {
        bool shaderChanged = _shader != shader;
        auto currentVertexSize = getVertices().size();
        auto currentIndexSize = getVertices().size();

        bool sizeChanged = _lastIndexSize != currentIndexSize || _lastVertexSize != currentVertexSize;

        if (shaderChanged || sizeChanged)
        {
            clear();
        }

        // Update our local shader reference
        _shader = shader;

        if (_shader)
        {
            _surfaceSlot = _shader->addSurface(*this);

            _lastVertexSize = currentVertexSize;
            _lastIndexSize = currentIndexSize;
        }
    }

    // Removes the surface and clears the shader reference
    void clear()
    {
        if (_shader && _surfaceSlot != ISurfaceRenderer::InvalidSlot)
        {
            _shader->removeSurface(_surfaceSlot);
        }

        _surfaceSlot = ISurfaceRenderer::InvalidSlot;
        _shader.reset();
    }

    // Renders the surface stored in our single slot
    void render(const RenderInfo& info) const override
    {
        if (_surfaceSlot != ISurfaceRenderer::InvalidSlot && _shader)
        {
            _shader->renderSurface(_surfaceSlot);
        }
    }
};

}
