#include "BlendLightProgram.h"

#include "debugging/gl.h"
#include "rendersystem/backend/GLProgramFactory.h"

namespace render
{

namespace
{
    // Filenames of shader code
    constexpr const char* const VP_FILENAME = "blend_light_vp.glsl";
    constexpr const char* const FP_FILENAME = "blend_light_fp.glsl";
}

void BlendLightProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL Blend Light program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(VP_FILENAME, FP_FILENAME);

    // Bind vertex attribute locations and link the program
    glBindAttribLocation(_programObj, GLProgramAttribute::Position, "attr_Position");

    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

    _locLightTextureMatrix = glGetUniformLocation(_programObj, "u_LightTextureMatrix");
    _locBlendColour = glGetUniformLocation(_programObj, "u_BlendColour");

    _locModelViewProjection = glGetUniformLocation(_programObj, "u_ModelViewProjection");
    _locObjectTransform = glGetUniformLocation(_programObj, "u_ObjectTransform");

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    auto samplerLoc = glGetUniformLocation(_programObj, "u_LightProjectionTexture");
    glUniform1i(samplerLoc, 0);
    samplerLoc = glGetUniformLocation(_programObj, "u_LightFallOffTexture");
    glUniform1i(samplerLoc, 1);

    debug::assertNoGlErrors();
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void BlendLightProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArray(GLProgramAttribute::Position);

    debug::assertNoGlErrors();
}

void BlendLightProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArray(GLProgramAttribute::Position);

    debug::assertNoGlErrors();
}

void BlendLightProgram::setModelViewProjection(const Matrix4& modelViewProjection)
{
    loadMatrixUniform(_locModelViewProjection, modelViewProjection);
}

void BlendLightProgram::setObjectTransform(const Matrix4& transform)
{
    loadMatrixUniform(_locObjectTransform, transform);
}

void BlendLightProgram::setLightTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locLightTextureMatrix, transform);
}

void BlendLightProgram::setBlendColour(const Vector4& colour)
{
    glUniform4dv(_locBlendColour, 4, colour);
}

}
