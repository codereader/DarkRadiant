#pragma once

#include <map>
#include <vector>
#include "isurfacerenderer.h"
#include "irender.h"

namespace render
{

/**
 * Surface base type, handling vertex data updates in combination
 * with one or more ISurfaceRenderer instances.
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
    using ShaderMapping = std::map<ShaderPtr, ISurfaceRenderer::Slot>;
    ShaderMapping _shaders;

protected:
    RenderableSurface()
    {}

public:
    // Noncopyable
    RenderableSurface(const RenderableSurface& other) = delete;
    RenderableSurface& operator=(const RenderableSurface& other) = delete;

    virtual ~RenderableSurface()
    {
        detach();
    }

    // (Non-virtual) update method handling any possible shader change
    // The surface is withdrawn from the given shader if it turns out
    // to be different from the last update.
    void attachToShader(const ShaderPtr& shader)
    {
        if (_shaders.count(shader) > 0)
        {
            return; // already attached
        }

        _shaders[shader] = shader->addSurface(*this);
    }

    // Notifies all the attached shaders that the surface geometry changed
    void queueUpdate()
    {
        for (auto& [shader, slot] : _shaders)
        {
            shader->updateSurface(slot);
        }
    }

    // Removes the surface from all shaders
    void detach()
    {
        while (!_shaders.empty())
        {
            detachFromShader(_shaders.begin());
        }
    }

    // Renders the surface stored in our single slot
    void render(const RenderInfo& info) const override
    {
        if (_shaders.empty()) return;

        auto& [shader, slot] = *_shaders.begin();
        shader->renderSurface(slot);
    }

private:
    void detachFromShader(const ShaderMapping::iterator& iter)
    {
        iter->first->removeSurface(iter->second);
        _shaders.erase(iter);
    }
};

}
