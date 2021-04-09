#pragma once

#include "OpenGLShaderPass.h"

namespace render
{

class OpenGLRenderSystem;

/**
 * Special render pass filling the depth buffer. 
 * This is the first pass before any interaction shaders are run.
 */
class DepthFillPass :
    public OpenGLShaderPass
{
public:
    DepthFillPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem);
};

}
