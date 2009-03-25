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

    // Vertex colour factor
    float _vertexColFactor;
	
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

    // Set render pass parameters
    void applyRenderParams(const Vector3& viewer, 
                           const Matrix4& localToWorld, 
                           const Vector3& origin, 
                           const Vector3& colour, 
                           const Matrix4& world2light,
                           float ambientFactor);
};

} // namespace render

#endif /*ARBBUMPPROGRAM_H_*/
