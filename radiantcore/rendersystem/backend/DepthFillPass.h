#pragma once

#include "OpenGLShaderPass.h"

namespace render
{

class OpenGLRenderSystem;
class GLProgramFactory;

/**
 * Special render pass filling the depth buffer. 
 * This is the first pass before any interaction shaders are run.
 */
class DepthFillPass :
    public OpenGLShaderPass
{
public:
    DepthFillPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem);

    // Returns the alpha test value of the diffuse stage, or -1.0f if none defined
    float getAlphaTestValue() const
    {
        const auto& diffuse = state().stage0;
        return diffuse && diffuse->hasAlphaTest() ? diffuse->getAlphaTest() : -1.0f;
    }

    Matrix4 getDiffuseTextureTransform() const
    {
        const auto& diffuse = state().stage0;
        return diffuse ? diffuse->getTextureTransform() : Matrix4::getIdentity();
    }

    // Generates the state with all the required flags for depth filling
    static OpenGLState GenerateDepthFillState(GLProgramFactory& programFactory);
};

}
