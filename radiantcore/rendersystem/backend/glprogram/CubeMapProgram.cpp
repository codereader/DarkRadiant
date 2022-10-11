#include "CubeMapProgram.h"

#include "itextstream.h"
#include "GLProgramAttributes.h"
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

void CubeMapProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL CubeMap program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(VP_FILENAME, FP_FILENAME);

    // Bind vertex attribute locations and link the program
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord0");
    glBindAttribLocation(_programObj, GLProgramAttribute::Tangent, "attr_Tangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Bitangent, "attr_Bitangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Normal, "attr_Normal");

    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

    // Get a grip of the uniform declared in the fragment shader
    _locViewOrigin = glGetUniformLocation(_programObj, "u_view_origin");

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    // Set the cube map sampler to texture unit 0
    auto samplerLoc = glGetUniformLocation(_programObj, "u_cubemap");
    glUniform1i(samplerLoc, 0);

    debug::assertNoGlErrors();
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void CubeMapProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Normal);

    debug::assertNoGlErrors();
}

void CubeMapProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Normal);

    debug::assertNoGlErrors();
}

void CubeMapProgram::setViewer(const Vector3& viewer)
{
    // Pass the current viewer origin to the shader
    glUniform3f(_locViewOrigin,
        static_cast<float>(viewer.x()),
        static_cast<float>(viewer.y()),
        static_cast<float>(viewer.z())
    );
    debug::assertNoGlErrors();
}

}
