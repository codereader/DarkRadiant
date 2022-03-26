#include "DepthFillPass.h"

#include "../OpenGLRenderSystem.h"
#include "GLProgramFactory.h"
#include "glprogram/DepthFillAlphaProgram.h"

namespace render
{

namespace
{

inline void setDepthFillStateFlags(OpenGLState& state, GLProgramFactory& programFactory)
{
    // Mask colour => we only write to the depth buffer
    state.setRenderFlag(RENDER_MASKCOLOUR);

    state.setRenderFlag(RENDER_FILL);
    state.setRenderFlag(RENDER_CULLFACE);
    state.setRenderFlag(RENDER_DEPTHTEST);
    state.setRenderFlag(RENDER_DEPTHWRITE);
    state.setRenderFlag(RENDER_PROGRAM);

    // Our shader will discard any fragments not passing the alphatest (if active)
    state.setRenderFlag(RENDER_ALPHATEST);

    // We need texture coords and the full vertex attribute stack
    state.setRenderFlag(RENDER_TEXTURE_2D);
    state.setRenderFlag(RENDER_BUMP);

    // ZFILL will make this pass pretty much top priority
    state.setSortPosition(OpenGLState::SORT_ZFILL);

    // Load the GLSL program tailored for this pass
    state.glProgram = programFactory.getBuiltInProgram(ShaderProgram::DepthFillAlpha);
    assert(dynamic_cast<DepthFillAlphaProgram*>(state.glProgram));
}

}

DepthFillPass::DepthFillPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem) :
    OpenGLShaderPass(owner)
{
    setDepthFillStateFlags(_glState, renderSystem.getGLProgramFactory());
}

OpenGLState DepthFillPass::GenerateDepthFillState(GLProgramFactory& programFactory)
{
    OpenGLState state;

    setDepthFillStateFlags(state, programFactory);

    return state;
}

}
