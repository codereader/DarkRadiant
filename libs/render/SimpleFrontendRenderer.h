#pragma once

#include "irenderable.h"
#include "irender.h"
#include <list>

namespace render
{

/**
 * greebo: This is a basic front-end renderer (collecting renderables)
 * No highlighting support. It is returning FullMaterials as renderer style.
 */
class SimpleFrontendRenderer :
    public RenderableCollector
{
public:
    SimpleFrontendRenderer()
    {}

    void addRenderable(Shader& shader,
                       const OpenGLRenderable& renderable,
                       const Matrix4& world, const LightSources* lights,
                       const IRenderEntity* entity) override
    {
        shader.addRenderable(renderable, world, lights, entity);
    }

    bool supportsFullMaterials() const override
    {
        return true;
    }

    // No support for selection highlighting
    void setHighlightFlag(Highlight::Flags flags, bool enabled) override {}
};

} // namespace
