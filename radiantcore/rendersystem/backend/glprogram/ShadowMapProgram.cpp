#include "ShadowMapProgram.h"

#include "GLProgramAttributes.h"
#include "../GLProgramFactory.h"
#include "debugging/gl.h"

#include "itextstream.h"

namespace render
{

namespace
{
    constexpr const char* const SHADOWMAP_VP_FILENAME = "shadowmap_vp.glsl";
    constexpr const char* const SHADOWMAP_FP_FILENAME = "shadowmap_fp.glsl";
}

void ShadowMapProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL shadowmap program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        SHADOWMAP_VP_FILENAME, SHADOWMAP_FP_FILENAME
    );

    glBindAttribLocation(_programObj, GLProgramAttribute::Position, "attr_Position");
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord");

    glLinkProgram(_programObj);

    debug::assertNoGlErrors();

    _locAlphaTest = glGetUniformLocation(_programObj, "u_AlphaTest");
    _locLightOrigin = glGetUniformLocation(_programObj, "u_LightOrigin");
    _locObjectTransform = glGetUniformLocation(_programObj, "u_ObjectTransform");
    _locDiffuseTextureMatrix = glGetUniformLocation(_programObj, "u_DiffuseTextureMatrix");

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    auto samplerLoc = glGetUniformLocation(_programObj, "u_Diffuse");
    glUniform1i(samplerLoc, 0);

    debug::assertNoGlErrors();
}

void ShadowMapProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArray(GLProgramAttribute::Position);
    glEnableVertexAttribArray(GLProgramAttribute::TexCoord);
}

void ShadowMapProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArray(GLProgramAttribute::Position);
    glDisableVertexAttribArray(GLProgramAttribute::TexCoord);
}

void ShadowMapProgram::setAlphaTest(float alphaTest)
{
    glUniform1f(_locAlphaTest, alphaTest);

    debug::assertNoGlErrors();
}

void ShadowMapProgram::setLightOrigin(const Vector3& lightOrigin)
{
    glUniform3f(_locLightOrigin,
        static_cast<GLfloat>(lightOrigin.x()), 
        static_cast<GLfloat>(lightOrigin.y()), 
        static_cast<GLfloat>(lightOrigin.z()));

    debug::assertNoGlErrors();
}

void ShadowMapProgram::setObjectTransform(const Matrix4& transform)
{
    loadMatrixUniform(_locObjectTransform, transform);
}

void ShadowMapProgram::setDiffuseTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locDiffuseTextureMatrix, transform);
}

}

