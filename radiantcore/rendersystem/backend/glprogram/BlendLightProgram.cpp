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
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord");
    glBindAttribLocation(_programObj, GLProgramAttribute::Tangent, "attr_Tangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Bitangent, "attr_Bitangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Normal, "attr_Normal");
    glBindAttribLocation(_programObj, GLProgramAttribute::Colour, "attr_Colour");

    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

#if 0
    _locDiffuseTextureMatrix = glGetUniformLocation(_programObj, "u_DiffuseTextureMatrix");
    _locColourModulation = glGetUniformLocation(_programObj, "u_ColourModulation");
    _locColourAddition = glGetUniformLocation(_programObj, "u_ColourAddition");
    _locModelViewProjection = glGetUniformLocation(_programObj, "u_ModelViewProjection");
    _locObjectTransform = glGetUniformLocation(_programObj, "u_ObjectTransform");
#endif

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    // Set the texture sampler to texture unit 0
    auto samplerLoc = glGetUniformLocation(_programObj, "u_Map");
    glUniform1i(samplerLoc, 0);

    debug::assertNoGlErrors();
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void BlendLightProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArray(GLProgramAttribute::Position);
    glEnableVertexAttribArray(GLProgramAttribute::TexCoord);
    glEnableVertexAttribArray(GLProgramAttribute::Tangent);
    glEnableVertexAttribArray(GLProgramAttribute::Bitangent);
    glEnableVertexAttribArray(GLProgramAttribute::Normal);
    glEnableVertexAttribArray(GLProgramAttribute::Colour);

    debug::assertNoGlErrors();
}

void BlendLightProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArray(GLProgramAttribute::Position);
    glDisableVertexAttribArray(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArray(GLProgramAttribute::Tangent);
    glDisableVertexAttribArray(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArray(GLProgramAttribute::Normal);
    glDisableVertexAttribArray(GLProgramAttribute::Colour);

    debug::assertNoGlErrors();
}

}
