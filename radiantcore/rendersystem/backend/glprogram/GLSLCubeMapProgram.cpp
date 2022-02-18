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
    glBindAttribLocation(_programObj, ATTR_TEXCOORD, "attr_TexCoord0");
    glBindAttribLocation(_programObj, ATTR_TANGENT, "attr_Tangent");
    glBindAttribLocation(_programObj, ATTR_BITANGENT, "attr_Bitangent");
    glBindAttribLocation(_programObj, ATTR_NORMAL, "attr_Normal");
    glLinkProgram(_programObj);
    debug::assertNoGlErrors();


}

void GLSLCubeMapProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArrayARB(ATTR_TEXCOORD);
    glEnableVertexAttribArrayARB(ATTR_TANGENT);
    glEnableVertexAttribArrayARB(ATTR_BITANGENT);
    glEnableVertexAttribArrayARB(ATTR_NORMAL);

    debug::assertNoGlErrors();
}

void GLSLCubeMapProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArrayARB(ATTR_TEXCOORD);
    glDisableVertexAttribArrayARB(ATTR_TANGENT);
    glDisableVertexAttribArrayARB(ATTR_BITANGENT);
    glDisableVertexAttribArrayARB(ATTR_NORMAL);

    debug::assertNoGlErrors();
}

void GLSLCubeMapProgram::applyRenderParams(const Vector3& viewer,
    const Matrix4& localToWorld,
    const Params&)
{

}

}
