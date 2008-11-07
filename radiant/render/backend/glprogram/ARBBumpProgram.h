#ifndef ARBBUMPPROGRAM_H_
#define ARBBUMPPROGRAM_H_

#include "GLProgramAttributes.h"
#include "iglrender.h"
#include "iregistry.h"
#include "iradiant.h"
#include "xmlutil/Document.h"
#include "math/matrix.h"
#include "render/backend/GLProgramFactory.h" 

namespace render {

/* CONSTANTS */
namespace {
    const char* BUMP_VP_FILENAME = "interaction_vp.arb";
    const char* BUMP_FP_FILENAME = "interaction_fp.arb";
}

class ARBBumpProgram 
: public GLProgram
{
private:

	// The value all lights should be scaled by, obtained from the game description
	double _lightScale;
	
public:
  GLuint m_vertex_program;
  GLuint m_fragment_program;

  void create()
  {

	// Initialise the lightScale value
	xml::NodeList scaleList = GlobalRegistry().findXPath("game/defaults/lightScale");
	if (scaleList.size() == 1) {
		std::stringstream stream(scaleList[0].getContent());
		stream >> _lightScale;
	}
	else {
		_lightScale = 1.0;
	}

    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);

    {
      glGenProgramsARB(1, &m_vertex_program);
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);


        // Create the vertex program
        GLProgramFactory::createARBProgram(
            GLProgramFactory::getGLProgramPath(BUMP_VP_FILENAME),
            GL_VERTEX_PROGRAM_ARB
        );

      glGenProgramsARB(1, &m_fragment_program);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);

        // Create the fragment program
        GLProgramFactory::createARBProgram(
            GLProgramFactory::getGLProgramPath(BUMP_FP_FILENAME),
            GL_FRAGMENT_PROGRAM_ARB
        );    
    }

    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    GlobalOpenGL_debugAssertNoErrors();
  }

  void destroy()
  {
    glDeleteProgramsARB(1, &m_vertex_program);
    glDeleteProgramsARB(1, &m_fragment_program);
    GlobalOpenGL_debugAssertNoErrors();
  }

  void enable()
  {
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);

    glEnableVertexAttribArrayARB(ATTR_TEXCOORD);
    glEnableVertexAttribArrayARB(ATTR_TANGENT);
    glEnableVertexAttribArrayARB(ATTR_BITANGENT);
    glEnableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL_debugAssertNoErrors();
  }

  void disable()
  {
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    glDisableVertexAttribArrayARB(ATTR_TEXCOORD);
    glDisableVertexAttribArrayARB(ATTR_TANGENT);
    glDisableVertexAttribArrayARB(ATTR_BITANGENT);
    glDisableVertexAttribArrayARB(ATTR_NORMAL);

    GlobalOpenGL_debugAssertNoErrors();
  }

  void setParameters(const Vector3& viewer, 
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
};

} // namespace render

#endif /*ARBBUMPPROGRAM_H_*/
