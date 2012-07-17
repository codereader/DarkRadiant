#include "ARBBumpProgram.h"
#include "render/backend/GLProgramFactory.h"

#include "igame.h"
#include "string/convert.h"
#include "math/Matrix4.h"

namespace render
{

namespace
{
    // Lightscale registry path
    const char* LOCAL_RKEY_LIGHTSCALE = "/defaults/lightScale";

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

}

// Main construction
void ARBBumpProgram::create()
{
	// Initialise the lightScale value
    game::IGamePtr currentGame = GlobalGameManager().currentGame();
    xml::NodeList scaleList = currentGame->getLocalXPath(LOCAL_RKEY_LIGHTSCALE);
	if (!scaleList.empty())
    {
		_lightScale = string::convert<double>(scaleList[0].getContent());
	}
	else {
		_lightScale = 1.0;
	}

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

    GlobalOpenGL().assertNoErrors();
}

void ARBBumpProgram::destroy()
{
    glDeleteProgramsARB(1, &m_vertex_program);
    glDeleteProgramsARB(1, &m_fragment_program);

    GlobalOpenGL().assertNoErrors();
}

void ARBBumpProgram::enable()
{
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);

    glEnableVertexAttribArrayARB(ATTR_TEXCOORD);
    glEnableVertexAttribArrayARB(ATTR_TANGENT);
    glEnableVertexAttribArrayARB(ATTR_BITANGENT);
    glEnableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL().assertNoErrors();
}

void ARBBumpProgram::disable()
{
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    glDisableVertexAttribArrayARB(ATTR_TEXCOORD);
    glDisableVertexAttribArrayARB(ATTR_TANGENT);
    glDisableVertexAttribArrayARB(ATTR_BITANGENT);
    glDisableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL().assertNoErrors();
}

void ARBBumpProgram::applyRenderParams(const Vector3& viewer,
                                       const Matrix4& objectToWorld,
                                       const Params& lp)
{
    Matrix4 worldToObject(objectToWorld);
    worldToObject.invert();

    // Calculate the light origin in object space
    Vector3 localLight = worldToObject.transformPoint(lp.lightOrigin);
    
    Matrix4 local2light(lp.world2Light);
    local2light.multiplyBy(objectToWorld); // local->world->light

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
        GL_FRAGMENT_PROGRAM_ARB, _locLightColour, 
        lp.lightColour.x(), lp.lightColour.y(), lp.lightColour.z(), 0
    );

	// light scale
	glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, _locLightScale, _lightScale, _lightScale, _lightScale, 0
    );

	// ambient factor
	glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, _locAmbientFactor, lp.ambientFactor, 0, 0, 0
    );

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    GlobalOpenGL().assertNoErrors();
}

}




