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
    game::IGamePtr currentGame = GlobalGameManager().currentGame();
    xml::NodeList scaleList = currentGame->getLocalXPath(LOCAL_RKEY_LIGHTSCALE);
	if (!scaleList.empty())
    {
		_lightScale = string::convert<float>(scaleList[0].getContent());
	}
	else {
		_lightScale = 1.0f;
	}

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
    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

    // Set the uniform locations to the correct bound values
    _locLightOrigin = glGetUniformLocation(_programObj, "u_light_origin");
    _locLightColour = glGetUniformLocation(_programObj, "u_light_color");
    _locViewOrigin = glGetUniformLocation(_programObj, "u_view_origin");
    _locLightScale = glGetUniformLocation(_programObj, "u_light_scale");
    _locInvertVCol = glGetUniformLocation(_programObj, "uInvertVCol");
    _locAmbientLight = glGetUniformLocation(_programObj, "uAmbientLight");

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

    glEnableVertexAttribArrayARB(GLProgramAttribute::Position);
    glEnableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glEnableVertexAttribArrayARB(GLProgramAttribute::Normal);

    debug::assertNoGlErrors();
}

void GLSLBumpProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArrayARB(GLProgramAttribute::Position);
    glDisableVertexAttribArrayARB(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Tangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArrayARB(GLProgramAttribute::Normal);

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

    // Set vertex colour parameters
    glUniform1i(_locInvertVCol, parms.invertVertexColour);

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    debug::assertNoGlErrors();
}

}




