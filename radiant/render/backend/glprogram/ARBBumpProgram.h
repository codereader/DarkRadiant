#ifndef ARBBUMPPROGRAM_H_
#define ARBBUMPPROGRAM_H_

#include "renderstate.h"
#include "iglrender.h"
#include "iregistry.h"
#include "qerplugin.h"
#include "xmlutil/Document.h"
#include "math/matrix.h"

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
      std::string filename = std::string(GlobalRadiant().getAppPath())
      						 + "gl/lighting_DBS_omni_vp.glp";
      createARBProgram(filename.c_str(), GL_VERTEX_PROGRAM_ARB);

      glGenProgramsARB(1, &m_fragment_program);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);

      filename = std::string(GlobalRadiant().getAppPath())
      			 + "gl/interaction_fp.arb";
      createARBProgram(filename.c_str(), GL_FRAGMENT_PROGRAM_ARB);
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

    glEnableVertexAttribArrayARB(8);
    glEnableVertexAttribArrayARB(9);
    glEnableVertexAttribArrayARB(10);
    glEnableVertexAttribArrayARB(11);

    GlobalOpenGL_debugAssertNoErrors();
  }

  void disable()
  {
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    glDisableVertexAttribArrayARB(8);
    glDisableVertexAttribArrayARB(9);
    glDisableVertexAttribArrayARB(10);
    glDisableVertexAttribArrayARB(11);

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

    Vector3 localLight(origin);
    matrix4_transform_point(world2local, localLight);

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
    glLoadMatrixf(reinterpret_cast<const float*>(&local2light));
    glMatrixMode(GL_MODELVIEW);

    GlobalOpenGL_debugAssertNoErrors();
  }
};



#endif /*ARBBUMPPROGRAM_H_*/
