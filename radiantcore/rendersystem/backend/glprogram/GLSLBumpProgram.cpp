#include "GLSLBumpProgram.h"
#include "../GLProgramFactory.h"

#include "itextstream.h"
#include "igame.h"
#include "string/convert.h"
#include "debugging/gl.h"
#include "math/Matrix4.h"

namespace render
{

namespace
{
    // Lightscale registry path
    const char* LOCAL_RKEY_LIGHTSCALE = "/defaults/lightScale";

    // Filenames of shader code
    const char* BUMP_VP_FILENAME = "interaction_vp.glsl";
    const char* BUMP_FP_FILENAME = "interaction_fp.glsl";

}

// Main construction
void GLSLBumpProgram::create()
{
	// Initialise the lightScale value
    auto currentGame = GlobalGameManager().currentGame();
    auto scaleList = currentGame->getLocalXPath(LOCAL_RKEY_LIGHTSCALE);
    _lightScale = !scaleList.empty() ? string::convert<float>(scaleList[0].getContent()) : 1.0f;

    // Create the program object
    rMessage() << "[renderer] Creating GLSL bump program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        BUMP_VP_FILENAME, BUMP_FP_FILENAME
    );

    // Bind vertex attribute locations and link the program
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord0");
    glBindAttribLocation(_programObj, GLProgramAttribute::Tangent, "attr_Tangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Bitangent, "attr_Bitangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Normal, "attr_Normal");
    glBindAttribLocation(_programObj, GLProgramAttribute::Colour, "attr_Colour");
    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

    // Set the uniform locations to the correct bound values
    _locLightOrigin = glGetUniformLocation(_programObj, "u_light_origin");
    _locLightColour = glGetUniformLocation(_programObj, "u_light_color");
    _locViewOrigin = glGetUniformLocation(_programObj, "u_view_origin");
    _locLightScale = glGetUniformLocation(_programObj, "u_light_scale");
    _locAmbientLight = glGetUniformLocation(_programObj, "uAmbientLight");
    _locColourModulation = glGetUniformLocation(_programObj, "u_colourModulation");
    _locColourAddition = glGetUniformLocation(_programObj, "u_colourAddition");
    _locObjectTransform = glGetUniformLocation(_programObj, "u_objectTransform");

    // Set up the texture uniforms. The renderer uses fixed texture units for
    // particular textures, so make sure they are correct here.
    // Texture 0 - diffuse
    // Texture 1 - bump
    // Texture 2 - specular
    // Texture 3 - XY attenuation map
    // Texture 4 - Z attenuation map

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    GLint samplerLoc;

    samplerLoc = glGetUniformLocation(_programObj, "u_diffusemap");
    glUniform1i(samplerLoc, 0);

    samplerLoc = glGetUniformLocation(_programObj, "u_bumpmap");
    glUniform1i(samplerLoc, 1);

    samplerLoc = glGetUniformLocation(_programObj, "u_specularmap");
    glUniform1i(samplerLoc, 2);

    samplerLoc = glGetUniformLocation(_programObj, "u_attenuationmap_xy");
    glUniform1i(samplerLoc, 3);

    samplerLoc = glGetUniformLocation(_programObj, "u_attenuationmap_z");
    glUniform1i(samplerLoc, 4);

    debug::assertNoGlErrors();
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void GLSLBumpProgram::enable()
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

void GLSLBumpProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArray(GLProgramAttribute::Position);
    glDisableVertexAttribArray(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArray(GLProgramAttribute::Tangent);
    glDisableVertexAttribArray(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArray(GLProgramAttribute::Normal);
    glDisableVertexAttribArray(GLProgramAttribute::Colour);

    // Switch back to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);

    debug::assertNoGlErrors();
}

void GLSLBumpProgram::applyRenderParams(const Vector3& viewer,
                                        const Matrix4& objectToWorld,
                                        const Params& parms)
{
    debug::assertNoGlErrors();

    Matrix4 worldToObject(objectToWorld);
    worldToObject.invert();

    // Calculate the light origin in object space
    Vector3 localLight = worldToObject.transformPoint(parms.lightOrigin);

    Matrix4 local2light(parms.world2Light);
    local2light.multiplyBy(objectToWorld); // local->world->light

    // Set lighting parameters in the shader
    glUniform3f(_locViewOrigin,
        static_cast<float>(viewer.x()),
        static_cast<float>(viewer.y()),
        static_cast<float>(viewer.z())
    );
    glUniform3f(_locLightOrigin,
        static_cast<float>(localLight.x()),
        static_cast<float>(localLight.y()),
        static_cast<float>(localLight.z())
    );
    glUniform3fv(_locLightColour, 1, parms.lightColour);
    glUniform1f(_locLightScale, _lightScale);
    glUniform1i(_locAmbientLight, parms.isAmbientLight);

    // Define the colour factors to blend into the final fragment
    switch (parms.vertexColourMode)
    {
    case IShaderLayer::VERTEX_COLOUR_NONE:
        // Nullify the vertex colour, add the stage colour as additive constant
        glUniform4f(_locColourModulation, 0, 0, 0, 0);
        glUniform4f(_locColourAddition,
            static_cast<float>(parms.stageColour.x()),
            static_cast<float>(parms.stageColour.y()),
            static_cast<float>(parms.stageColour.z()),
            static_cast<float>(parms.stageColour.w()));
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

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    debug::assertNoGlErrors();
}

void GLSLBumpProgram::setObjectTransform(const Matrix4& transform)
{
    loadMatrixUniform(_locObjectTransform, transform);
}

}




