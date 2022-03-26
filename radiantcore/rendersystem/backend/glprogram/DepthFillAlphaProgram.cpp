#include "DepthFillAlphaProgram.h"

#include "GLProgramAttributes.h"
#include "../GLProgramFactory.h"
#include "debugging/gl.h"

#include "itextstream.h"

namespace render
{

namespace
{
    const char* DEPTHFILL_ALPHA_VP_FILENAME = "zfill_alpha_vp.glsl";
    const char* DEPTHFILL_ALPHA_FP_FILENAME = "zfill_alpha_fp.glsl";
}

void DepthFillAlphaProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL depthfill+alpha program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        DEPTHFILL_ALPHA_VP_FILENAME, DEPTHFILL_ALPHA_FP_FILENAME
    );

    glBindAttribLocation(_programObj, GLProgramAttribute::Position, "attr_Position");
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord");

    glLinkProgram(_programObj);

    debug::assertNoGlErrors();

    _locAlphaTest = glGetUniformLocation(_programObj, "u_AlphaTest");
    _locObjectTransform = glGetUniformLocation(_programObj, "u_ObjectTransform");
    _locModelViewProjection = glGetUniformLocation(_programObj, "u_ModelViewProjection");
    _locDiffuseTextureMatrix = glGetUniformLocation(_programObj, "u_DiffuseTextureMatrix");

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    auto samplerLoc = glGetUniformLocation(_programObj, "u_Diffuse");
    glUniform1i(samplerLoc, 0);

    debug::assertNoGlErrors();
}

void DepthFillAlphaProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArray(GLProgramAttribute::Position);
    glEnableVertexAttribArray(GLProgramAttribute::TexCoord);
}

void DepthFillAlphaProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArray(GLProgramAttribute::Position);
    glDisableVertexAttribArray(GLProgramAttribute::TexCoord);
}

void DepthFillAlphaProgram::setAlphaTest(float alphaTest)
{
    glUniform1f(_locAlphaTest, alphaTest);
}

void DepthFillAlphaProgram::setModelViewProjection(const Matrix4& modelViewProjection)
{
    loadMatrixUniform(_locModelViewProjection, modelViewProjection);
}

void DepthFillAlphaProgram::setObjectTransform(const Matrix4& transform)
{
    loadMatrixUniform(_locObjectTransform, transform);
}

void DepthFillAlphaProgram::setDiffuseTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locDiffuseTextureMatrix, transform);
}

}

