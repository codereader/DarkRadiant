#pragma once

#include "OpenGLShaderPass.h"
#include "glprogram/GLSLBumpProgram.h"

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

    GLSLBumpProgram& getProgram()
    {
        return *static_cast<GLSLBumpProgram*>(_glState.glProgram);
    }
};

}
