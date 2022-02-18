#include "DepthFillPass.h"

#include "../OpenGLRenderSystem.h"
#include "GLProgramFactory.h"
#include "glprogram/GLSLDepthFillAlphaProgram.h"

namespace render
{

DepthFillPass::DepthFillPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem) :
    OpenGLShaderPass(owner)
{
    // Mask colour => we only write to the depth buffer
    _glState.setRenderFlag(RENDER_MASKCOLOUR);

    _glState.setRenderFlag(RENDER_FILL);
    _glState.setRenderFlag(RENDER_CULLFACE);
    _glState.setRenderFlag(RENDER_DEPTHTEST);
    _glState.setRenderFlag(RENDER_DEPTHWRITE);
    _glState.setRenderFlag(RENDER_PROGRAM);
    
    // Our shader will discard any fragments not passing the alphatest (if active)
    _glState.setRenderFlag(RENDER_ALPHATEST);
    
    // We need texture coords and the full vertex attribute stack
    _glState.setRenderFlag(RENDER_TEXTURE_2D);
    _glState.setRenderFlag(RENDER_BUMP);

    // ZFILL will make this pass pretty much top priority
    _glState.setSortPosition(OpenGLState::SORT_ZFILL);

    // Load the GLSL program tailored for this pass
    _glState.glProgram = renderSystem.getGLProgramFactory().getBuiltInProgram(ShaderProgram::DepthFillAlpha);
    assert(dynamic_cast<GLSLDepthFillAlphaProgram*>(_glState.glProgram));
}

void DepthFillPass::activateShaderProgram(OpenGLState& current)
{
    // We need a program, it is set up in the constructor
    assert(_glState.glProgram);

    // Let the base class enable the program
    OpenGLShaderPass::activateShaderProgram(current);

    auto zFillAlphaProgram = static_cast<GLSLDepthFillAlphaProgram*>(current.glProgram);

    zFillAlphaProgram->applyAlphaTest(_glState.alphaThreshold);

    setTextureState(current.texture0, _glState.texture0, GL_TEXTURE0, GL_TEXTURE_2D);
    setupTextureMatrix(GL_TEXTURE0, _glState.stage0);
}

}
