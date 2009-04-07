#include "ARBBumpProgram.h"

namespace render
{

namespace
{
    /* CONSTANT FRAGMENT PROGRAM PARAMETERS
     * These should match what is used by interaction_fp.cg
     */
    const int C2_LIGHT_ORIGIN = 2;
    const int C3_LIGHT_COLOR = 3;
    const int C4_VIEW_ORIGIN = 4;
    const int C6_LIGHT_SCALE = 6;
    const int C7_AMBIENT_FACTOR = 7;

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

    // view origin
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C4_VIEW_ORIGIN, viewer.x(), viewer.y(), viewer.z(), 0
    );

    // light origin
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C2_LIGHT_ORIGIN, localLight.x(), localLight.y(), localLight.z(), 1
    );

    // light colour
    glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C3_LIGHT_COLOR, colour.x(), colour.y(), colour.z(), 0
    );

	// light scale
	glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C6_LIGHT_SCALE, _lightScale, _lightScale, _lightScale, 0
    );

	// ambient factor
	glProgramLocalParameter4fARB(
        GL_FRAGMENT_PROGRAM_ARB, C7_AMBIENT_FACTOR, ambientFactor, 0, 0, 0
    );

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    GlobalOpenGL_debugAssertNoErrors();
}

}




