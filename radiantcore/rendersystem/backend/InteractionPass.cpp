#include "InteractionPass.h"

#include "GLProgramFactory.h"
#include "../OpenGLRenderSystem.h"

namespace render
{

InteractionPass::InteractionPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem) :
    OpenGLShaderPass(owner)
{
    // Set render flags
    _glState.setRenderFlag(RENDER_BLEND);
    _glState.setRenderFlag(RENDER_FILL);
    _glState.setRenderFlag(RENDER_TEXTURE_2D);
    _glState.setRenderFlag(RENDER_CULLFACE);
    _glState.setRenderFlag(RENDER_DEPTHTEST);
    _glState.setRenderFlag(RENDER_SMOOTH);
    _glState.setRenderFlag(RENDER_BUMP);
    _glState.setRenderFlag(RENDER_PROGRAM);

    _glState.glProgram = renderSystem.getGLProgramFactory().getBuiltInProgram(ShaderProgram::Interaction);

    _glState.setDepthFunc(GL_LEQUAL);
    _glState.polygonOffset = 0.5f;
    _glState.setSortPosition(OpenGLState::SORT_INTERACTION);
    _glState.m_blend_src = GL_ONE;
    _glState.m_blend_dst = GL_ONE;
}

}
