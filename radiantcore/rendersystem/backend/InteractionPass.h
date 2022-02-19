#pragma once

#include "OpenGLShaderPass.h"

namespace render
{

class OpenGLRenderSystem;

/**
 * Lighting Interaction pass (Diffuse/Bump/Specular).
 */
class InteractionPass :
    public OpenGLShaderPass
{
public:
    InteractionPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem);
};

}
