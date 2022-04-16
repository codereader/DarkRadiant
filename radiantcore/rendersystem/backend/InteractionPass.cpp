#include "InteractionPass.h"

#include "GLProgramFactory.h"
#include "../OpenGLRenderSystem.h"
#include "glprogram/InteractionProgram.h"

namespace render
{

inline void setInteractionStateFlags(OpenGLState& state, GLProgramFactory& programFactory)
{
    // Set render flags
    state.setRenderFlag(RENDER_BLEND);
    state.setRenderFlag(RENDER_FILL);
    state.setRenderFlag(RENDER_TEXTURE_2D);
    state.setRenderFlag(RENDER_CULLFACE);
    state.setRenderFlag(RENDER_DEPTHTEST);
    state.setRenderFlag(RENDER_SMOOTH);
    state.setRenderFlag(RENDER_BUMP);
    state.setRenderFlag(RENDER_PROGRAM);

    state.glProgram = programFactory.getBuiltInProgram(ShaderProgram::Interaction);
    assert(dynamic_cast<InteractionProgram*>(state.glProgram));

    state.setDepthFunc(GL_LEQUAL);
    state.setSortPosition(OpenGLState::SORT_INTERACTION);
    state.m_blend_src = GL_ONE;
    state.m_blend_dst = GL_ONE;
}

InteractionPass::InteractionPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem) :
    OpenGLShaderPass(owner)
{
    setInteractionStateFlags(_glState, renderSystem.getGLProgramFactory());
}

OpenGLState InteractionPass::GenerateInteractionState(GLProgramFactory& programFactory)
{
    OpenGLState state;

    setInteractionStateFlags(state, programFactory);

    return state;
}

}
