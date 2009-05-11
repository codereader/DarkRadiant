#include "ARBBumpProgram.h"
#include "render/backend/GLProgramFactory.h"

#include "igame.h"
#include "string/string.h"

namespace render
{

namespace
{
    // Lightscale registry path
    const char* LOCAL_RKEY_LIGHTSCALE = "/defaults/lightScale";

#ifdef RADIANT_USE_GLSL

    // Filenames of shader code
    const char* BUMP_VP_FILENAME = "interaction_vp.glsl";
    const char* BUMP_FP_FILENAME = "interaction_fp.glsl";

#else

    // Filenames of shader code
    const char* BUMP_VP_FILENAME = "interaction_vp.arb";
    const char* BUMP_FP_FILENAME = "interaction_fp.arb";

    /* CONSTANT FRAGMENT PROGRAM PARAMETERS
     * These should match what is used by interaction_fp.cg
     */
    const int C2_LIGHT_ORIGIN = 2;
    const int C3_LIGHT_COLOR = 3;
    const int C4_VIEW_ORIGIN = 4;
    const int C6_LIGHT_SCALE = 6;
    const int C7_AMBIENT_FACTOR = 7;

#endif

}

// Main construction
void ARBBumpProgram::create()
{
	// Initialise the lightScale value
    game::IGamePtr currentGame = GlobalGameManager().currentGame();
    xml::NodeList scaleList = currentGame->getLocalXPath(LOCAL_RKEY_LIGHTSCALE);
	if (!scaleList.empty()) 
    {
		_lightScale = strToDouble(scaleList[0].getContent());
	}
	else {
		_lightScale = 1.0;
	}

#ifdef RADIANT_USE_GLSL

    // Create the program object
    std::cout << "[renderer] Creating GLSL bump program" << std::endl;
    _programObj = GLProgramFactory::createGLSLProgram(
        BUMP_VP_FILENAME, BUMP_FP_FILENAME
    );

    // Bind vertex attribute locations and link the program
    glBindAttribLocation(_programObj, ATTR_TEXCOORD, "attr_TexCoord0");
    glBindAttribLocation(_programObj, ATTR_TANGENT, "attr_Tangent");
    glBindAttribLocation(_programObj, ATTR_BITANGENT, "attr_Bitangent");
    glBindAttribLocation(_programObj, ATTR_NORMAL, "attr_Normal");
    glLinkProgram(_programObj);
    GlobalOpenGL_debugAssertNoErrors();

    // Set the uniform locations to the correct bound values
    _locLightOrigin = glGetUniformLocation(_programObj, "u_light_origin");
    _locLightColour = glGetUniformLocation(_programObj, "u_light_color");
    _locViewOrigin = glGetUniformLocation(_programObj, "u_view_origin");
    _locLightScale = glGetUniformLocation(_programObj, "u_light_scale");

    // Set up the texture uniforms. The renderer uses fixed texture units for
    // particular textures, so make sure they are correct here.
    // Texture 0 - diffuse
    // Texture 1 - bump
    // Texture 2 - specular
    // Texture 3 - XY attenuation map
    // Texture 4 - Z attenuation map
    
    glUseProgram(_programObj);
    GlobalOpenGL_debugAssertNoErrors();

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

    GlobalOpenGL_debugAssertNoErrors();
    glUseProgram(0);

    GlobalOpenGL_debugAssertNoErrors();

#else

    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);

    // Create the vertex program
    m_vertex_program = GLProgramFactory::createARBProgram(
        BUMP_VP_FILENAME, GL_VERTEX_PROGRAM_ARB
    );

    // Create the fragment program
    m_fragment_program = GLProgramFactory::createARBProgram(
        BUMP_FP_FILENAME, GL_FRAGMENT_PROGRAM_ARB
    );    

    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    // Set the uniform locations to constants
    _locLightOrigin = C2_LIGHT_ORIGIN;
    _locLightColour = C3_LIGHT_COLOR;
    _locViewOrigin = C4_VIEW_ORIGIN;
    _locLightScale = C6_LIGHT_SCALE;
    _locAmbientFactor = C7_AMBIENT_FACTOR;

#endif

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBBumpProgram::destroy()
{
#ifdef RADIANT_USE_GLSL
    glDeleteProgram(_programObj);
#else
    glDeleteProgramsARB(1, &m_vertex_program);
    glDeleteProgramsARB(1, &m_fragment_program);
#endif

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBBumpProgram::enable()
{
#ifdef RADIANT_USE_GLSL
    glUseProgram(_programObj);
#else
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);
#endif

    glEnableVertexAttribArrayARB(ATTR_TEXCOORD);
    glEnableVertexAttribArrayARB(ATTR_TANGENT);
    glEnableVertexAttribArrayARB(ATTR_BITANGENT);
    glEnableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBBumpProgram::disable()
{
#ifdef RADIANT_USE_GLSL
    glUseProgram(0);
#else
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
#endif

    glDisableVertexAttribArrayARB(ATTR_TEXCOORD);
    glDisableVertexAttribArrayARB(ATTR_TANGENT);
    glDisableVertexAttribArrayARB(ATTR_BITANGENT);
    glDisableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBBumpProgram::applyRenderParams(const Vector3& viewer, 
                                       const Matrix4& objectToWorld, 
                                       const Vector3& origin, 
                                       const Vector3& colour, 
                                       const Matrix4& world2light,
                                       float ambientFactor)
{
    Matrix4 worldToObject(objectToWorld);
    matrix4_affine_invert(worldToObject);

    // Calculate the light origin in object space
    Vector3 localLight(origin);
    matrix4_transform_point(worldToObject, localLight);

    Matrix4 local2light(world2light);
    local2light.multiplyBy(objectToWorld); // local->world->light

#ifdef RADIANT_USE_GLSL

    // Bind uniform parameters
    glUniform3f(
        _locViewOrigin, viewer.x(), viewer.y(), viewer.z()
    );
    glUniform3f(
        _locLightOrigin, localLight.x(), localLight.y(), localLight.z()
    );
    glUniform3f(
        _locLightColour, colour.x(), colour.y(), colour.z()
    );
    glUniform1f(_locLightScale, _lightScale);

#else

    // view origin
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, _locViewOrigin, viewer.x(), viewer.y(), viewer.z(), 0
    );

    // light origin
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, _locLightOrigin, localLight.x(), localLight.y(), localLight.z(), 1
    );

    // light colour
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, _locLightColour, colour.x(), colour.y(), colour.z(), 0
    );

	// light scale
	glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, _locLightScale, _lightScale, _lightScale, _lightScale, 0
    );

	// ambient factor
	glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, _locAmbientFactor, ambientFactor, 0, 0, 0
    );

#endif

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    GlobalOpenGL_debugAssertNoErrors();
}

}




