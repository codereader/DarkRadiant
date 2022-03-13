#pragma once

#include "OpenGLShaderPass.h"
#include "glprogram/GLSLBumpProgram.h"

namespace render
{

class OpenGLRenderSystem;
class GLProgramFactory;

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

    // Generates the state with all the required flags for drawing interaction passes
    static OpenGLState GenerateInteractionState(GLProgramFactory& programFactory);
};

}
