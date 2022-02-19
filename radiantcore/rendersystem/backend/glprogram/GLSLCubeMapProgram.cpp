#include "GLSLCubeMapProgram.h"

#include "itextstream.h"
#include "debugging/gl.h"
#include "igl.h"
#include "../GLProgramFactory.h"

namespace render
{

namespace
{
    // Filenames of shader code
    const char* const VP_FILENAME = "cubemap_vp.glsl";
    const char* const FP_FILENAME = "cubemap_fp.glsl";
}

void GLSLCubeMapProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL CubeMap program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(VP_FILENAME, FP_FILENAME);

    // Bind vertex attribute locations and link the program
    glBindAttribLocation(_programObj, GLProgramAttribute::Position, "attr_Position");
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord0");
    glBindAttribLocation(_programObj, GLProgramAttribute::Tangent, "attr_Tangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Bitangent, "attr_Bitangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Normal, "attr_Normal");

    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

    _locViewOrigin = glGetUniformLocation(_programObj, "u_viewOrigin");
}

void GLSLCubeMapProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArrayARB(GLProgramAttribute::Position);
    glEnableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Normal);

    glUniform3f(_locViewOrigin,
        0,
        0,
        0
    );

    debug::assertNoGlErrors();
}

void GLSLCubeMapProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArrayARB(GLProgramAttribute::Position);
    glDisableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Normal);

    debug::assertNoGlErrors();
}

void GLSLCubeMapProgram::applyRenderParams(const Vector3& viewer,
    const Matrix4& localToWorld,
    const Params&)
{

}

}
