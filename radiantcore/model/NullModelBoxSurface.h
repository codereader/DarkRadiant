#pragma once

#include "irender.h"
#include "RenderableModelSurface.h"

namespace model
{

class NullModelBoxSurface :
    public RenderableModelSurface
{
public:
    NullModelBoxSurface(const IIndexedModelSurface& surface, const IRenderEntity* entity, const Matrix4& localToWorld) :
        RenderableModelSurface(surface, entity, localToWorld)
    {}

    ShaderPtr captureWireShader(RenderSystem& renderSystem) override
    {
        return renderSystem.capture(ColourShaderType::OrthoviewSolid, { 1.0f, 0, 0, 1 });
    }

    ShaderPtr captureFillShader(RenderSystem& renderSystem) override
    {
        return renderSystem.capture(BuiltInShaderType::MissingModel);
    }
};

}
