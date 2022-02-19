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
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord0");
    glBindAttribLocation(_programObj, GLProgramAttribute::Tangent, "attr_Tangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Bitangent, "attr_Bitangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Normal, "attr_Normal");

    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

    // Set the uniform locations to the correct bound values
    _locViewOrigin = glGetUniformLocation(_programObj, "u_view_origin");

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    GLint samplerLoc;

    //samplerLoc = glGetUniformLocation(_programObj, "u_diffusemap");
    //glUniform1i(samplerLoc, 0);
    //
    //samplerLoc = glGetUniformLocation(_programObj, "u_bumpmap");
    //glUniform1i(samplerLoc, 1);
    //
    //samplerLoc = glGetUniformLocation(_programObj, "u_specularmap");
    //glUniform1i(samplerLoc, 2);
    //
    //samplerLoc = glGetUniformLocation(_programObj, "u_attenuationmap_xy");
    //glUniform1i(samplerLoc, 3);
    //
    //samplerLoc = glGetUniformLocation(_programObj, "u_attenuationmap_z");
    //glUniform1i(samplerLoc, 4);
    
    // Texture 0 => cubemap
    samplerLoc = glGetUniformLocation(_programObj, "u_cubemap");
    glUniform1i(samplerLoc, 0);

    debug::assertNoGlErrors();
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void GLSLCubeMapProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Normal);

    debug::assertNoGlErrors();
}

void GLSLCubeMapProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Normal);

    debug::assertNoGlErrors();
}

void GLSLCubeMapProgram::applyRenderParams(const Vector3& viewer,
    const Matrix4& objectToWorld,
    const Params& parms)
{
#if 0
    debug::assertNoGlErrors();

    Matrix4 worldToObject(objectToWorld);
    worldToObject.invert();

    // Calculate the light origin in object space
    Vector3 localLight = worldToObject.transformPoint(parms.lightOrigin);

    Matrix4 local2light(parms.world2Light);
    local2light.multiplyBy(objectToWorld); // local->world->light
#endif
    // Set lighting parameters in the shader
    glUniform3f(_locViewOrigin,
        static_cast<float>(viewer.x()),
        static_cast<float>(viewer.y()),
        static_cast<float>(viewer.z())
    );
    debug::assertNoGlErrors();
#if 0
    glUniform1i(_locAmbientLight, parms.isAmbientLight);

    // Set vertex colour parameters
    glUniform1i(_locInvertVCol, parms.invertVertexColour);

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    debug::assertNoGlErrors();
#endif
}

}
