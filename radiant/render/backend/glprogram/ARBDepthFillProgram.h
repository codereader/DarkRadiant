#ifndef ARBDEPTHFILLPROGRAM_H_
#define ARBDEPTHFILLPROGRAM_H_

#include "render/backend/GLProgramFactory.h"

namespace render {

class ARBDepthFillProgram : 
	public GLProgram
{
public:
  GLuint m_vertex_program;
  GLuint m_fragment_program;

  void create()
  {
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);

    {
      glGenProgramsARB(1, &m_vertex_program);
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);
      std::string filename = GlobalRegistry().get("user/paths/appPath")
      						 + "gl/zfill_vp.glp";
      GLProgramFactory::createARBProgram(filename, GL_VERTEX_PROGRAM_ARB);

      glGenProgramsARB(1, &m_fragment_program);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);
      
      filename = GlobalRegistry().get("user/paths/appPath")
      			 + "gl/zfill_fp.glp";
      GLProgramFactory::createARBProgram(filename, GL_FRAGMENT_PROGRAM_ARB);
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

    GlobalOpenGL_debugAssertNoErrors();
  }

  void disable()
  {
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    GlobalOpenGL_debugAssertNoErrors();
  }

  void setParameters(const Vector3& viewer, const Matrix4& localToWorld, const Vector3& origin, const Vector3& colour, const Matrix4& world2light, float ambient)
  {
  }
};

} // namespace render

#endif /*ARBDEPTHFILLPROGRAM_H_*/
