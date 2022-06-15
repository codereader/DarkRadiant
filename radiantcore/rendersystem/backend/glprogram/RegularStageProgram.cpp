#include "RegularStageProgram.h"

#include "itextstream.h"
#include "debugging/gl.h"
#include "igl.h"
#include "../GLProgramFactory.h"

namespace render
{

namespace
{
    // Filenames of shader code
    const char* const VP_FILENAME = "regular_stage_vp.glsl";
    const char* const FP_FILENAME = "regular_stage_fp.glsl";
}

void RegularStageProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL Regular Stage program" << std::endl;

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

    _locDiffuseTextureMatrix = glGetUniformLocation(_programObj, "u_DiffuseTextureMatrix");
    _locColourModulation = glGetUniformLocation(_programObj, "u_ColourModulation");
    _locColourAddition = glGetUniformLocation(_programObj, "u_ColourAddition");
    _locModelViewProjection = glGetUniformLocation(_programObj, "u_ModelViewProjection");
    _locObjectTransform = glGetUniformLocation(_programObj, "u_ObjectTransform");

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    // Set the texture sampler to texture unit 0
    auto samplerLoc = glGetUniformLocation(_programObj, "u_Map");
    glUniform1i(samplerLoc, 0);

    debug::assertNoGlErrors();
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void RegularStageProgram::enable()
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

void RegularStageProgram::disable()
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

void RegularStageProgram::setStageVertexColour(IShaderLayer::VertexColourMode vertexColourMode, const Colour4& stageColour)
{
    // Define the colour factors to blend into the final fragment
    switch (vertexColourMode)
    {
    case IShaderLayer::VERTEX_COLOUR_NONE:
        // Nullify the vertex colour, add the stage colour as additive constant
        glUniform4f(_locColourModulation, 0, 0, 0, 0);
        glUniform4f(_locColourAddition,
            static_cast<float>(stageColour.x()),
            static_cast<float>(stageColour.y()),
            static_cast<float>(stageColour.z()),
            static_cast<float>(stageColour.w()));
        break;

    case IShaderLayer::VERTEX_COLOUR_MULTIPLY:
        // Multiply the fragment with 1*vertexColour
        glUniform4f(_locColourModulation, 1, 1, 1, 1);
        glUniform4f(_locColourAddition, 0, 0, 0, 0);
        break;

    case IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY:
        // Multiply the fragment with (1 - vertexColour)
        glUniform4f(_locColourModulation, -1, -1, -1, -1);
        glUniform4f(_locColourAddition, 1, 1, 1, 1);
        break;
    }
}

void RegularStageProgram::setModelViewProjection(const Matrix4& modelViewProjection)
{
    loadMatrixUniform(_locModelViewProjection, modelViewProjection);
}

void RegularStageProgram::setObjectTransform(const Matrix4& transform)
{
    loadMatrixUniform(_locObjectTransform, transform);
}

void RegularStageProgram::setDiffuseTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locDiffuseTextureMatrix, transform);
}

}
