#include "GLSLDepthFillAlphaProgram.h"

#include "GLProgramAttributes.h"
#include "../GLProgramFactory.h"
#include "debugging/gl.h"

#include "itextstream.h"

namespace render
{

namespace
{
    const char* DEPTHFILL_ALPHA_VP_FILENAME = "zfill_vp.glsl"; // use the same VP
    const char* DEPTHFILL_ALPHA_FP_FILENAME = "zfill_alpha_fp.glsl";
}

void GLSLDepthFillAlphaProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL depthfill+alpha program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        DEPTHFILL_ALPHA_VP_FILENAME, DEPTHFILL_ALPHA_FP_FILENAME
    );

    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord0");

    glLinkProgram(_programObj);

    debug::assertNoGlErrors();

    _locAlphaTest = glGetUniformLocation(_programObj, "u_alpha_test");

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    GLint samplerLoc = glGetUniformLocation(_programObj, "u_diffuse");
    glUniform1i(samplerLoc, 0);

    debug::assertNoGlErrors();
}

void GLSLDepthFillAlphaProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
}

void GLSLDepthFillAlphaProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
}

void GLSLDepthFillAlphaProgram::applyAlphaTest(float alphaTest)
{
    glUniform1f(_locAlphaTest, alphaTest);

    debug::assertNoGlErrors();

    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    
    debug::assertNoGlErrors();
}

}

