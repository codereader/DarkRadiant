#include "ARBBumpProgram.h"

namespace render
{

void ARBBumpProgram::setParameters(const Vector3& viewer, 
                                   const Matrix4& localToWorld, 
                                   const Vector3& origin, 
                                   const Vector3& colour, 
                                   const Matrix4& world2light,
                                   float ambientFactor)
{
    Matrix4 world2local(localToWorld);
    matrix4_affine_invert(world2local);

    // Calculate the light origin in object space
    Vector3 localLight(origin);
    matrix4_transform_point(world2local, localLight);

    // Viewer location in object space
    Vector3 localViewer(viewer);
    matrix4_transform_point(world2local, localViewer);

    Matrix4 local2light(world2light);
    matrix4_multiply_by_matrix4(local2light, localToWorld); // local->world->light

    // view origin
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 4, localViewer.x(), localViewer.y(), localViewer.z(), 0);

    // light origin
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, localLight.x(), localLight.y(), localLight.z(), 1);

    // light colour
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 3, colour.x(), colour.y(), colour.z(), 0);

    // bump scale
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, 1, 0, 0, 0);

    // specular exponent
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 5, 32, 0, 0, 0);

	// light scale
	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 6, _lightScale, _lightScale, _lightScale, 0);

	// ambient factor
	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 7, ambientFactor, 0, 0, 0);

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    GlobalOpenGL_debugAssertNoErrors();
}

}




