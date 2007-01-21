#include "OpenGLStateBucket.h"
#include "OpenGLShader.h"

#include "renderstate.h"
#include "math/matrix.h"
#include "math/aabb.h"
#include "irender.h"
#include "ishaders.h"
#include "texturelib.h"

/* Externals */
extern Shader* g_defaultPointLight; // renderstate.cpp, TODO: get rid of this

namespace {
	
// Utility function to set OpenGL texture state
inline void setTextureState(GLint& current, 
							const GLint& texture, 
							GLenum textureUnit)
{
  if(texture != current)
  {
    glActiveTexture(textureUnit);
    glClientActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
    GlobalOpenGL_debugAssertNoErrors();
    current = texture;
  }
}

}

// Render the bucket contents
void OpenGLStateBucket::render(OpenGLState& current, unsigned int globalstate, const Vector3& viewer)
{
  if((globalstate & m_state.m_state & RENDER_SCREEN) != 0)
  {
    OpenGLState_apply(m_state, current, globalstate);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(reinterpret_cast<const float*>(&g_matrix4_identity));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(reinterpret_cast<const float*>(&g_matrix4_identity));

    glBegin(GL_QUADS);
    glVertex3f(-1, -1, 0);
    glVertex3f(1, -1, 0);
    glVertex3f(1, 1, 0);
    glVertex3f(-1, 1, 0);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }
  else if(!m_renderables.empty())
  {
    OpenGLState_apply(m_state, current, globalstate);
    flushRenderables(current, globalstate, viewer);
  }
}

// Flush renderables
void OpenGLStateBucket::flushRenderables(OpenGLState& current, 
										 unsigned int globalstate, 
										 const Vector3& viewer)
{
	const Matrix4* transform = 0;
	glPushMatrix();
	for(OpenGLStateBucket::Renderables::const_iterator i = m_renderables.begin(); 
  	  	i != m_renderables.end(); 
  	  	++i)
	{
    //qglLoadMatrixf(i->m_transform);
    if(!transform || (transform != (*i).m_transform && !matrix4_affine_equal(*transform, *(*i).m_transform)))
    {
      transform = (*i).m_transform;
      glPopMatrix();
      glPushMatrix();
      glMultMatrixf(reinterpret_cast<const float*>(transform));
      glFrontFace(((current.m_state & RENDER_CULLFACE) != 0 && matrix4_handedness(*transform) == MATRIX4_RIGHTHANDED) ? GL_CW : GL_CCW);
    }

    if(current.m_program != 0 && (*i).m_light != 0)
    {
      const IShader* lightShader = i->m_light->getShader()->getIShader();
      if(lightShader->firstLayer() != 0)
      {
        GLuint attenuation_xy = lightShader->firstLayer()->texture()->texture_number;
        GLuint attenuation_z = lightShader->lightFalloffImage() != 0
          ? lightShader->lightFalloffImage()->texture_number
          : static_cast<OpenGLShader*>(g_defaultPointLight)->getShader().lightFalloffImage()->texture_number;

        setTextureState(current.m_texture3, attenuation_xy, GL_TEXTURE3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, attenuation_xy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        setTextureState(current.m_texture4, attenuation_z, GL_TEXTURE4);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, attenuation_z);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


        AABB lightBounds((*i).m_light->aabb());

        Matrix4 world2light(g_matrix4_identity);

        if (i->m_light->isProjected()) {
          world2light = (*i).m_light->projection();
          matrix4_multiply_by_matrix4(world2light, i->m_light->rotation().getTransposed());
          
          // greebo: old code: matrix4_translate_by_vec3(world2light, -lightBounds.origin); // world->lightBounds
          matrix4_translate_by_vec3(world2light, -(i->m_light->offset()));
        }
        else {
          matrix4_translate_by_vec3(world2light, Vector3(0.5f, 0.5f, 0.5f));
          matrix4_scale_by_vec3(world2light, Vector3(0.5f, 0.5f, 0.5f));
          matrix4_scale_by_vec3(world2light, Vector3(1.0f / lightBounds.extents.x(), 1.0f / lightBounds.extents.y(), 1.0f / lightBounds.extents.z()));
          matrix4_multiply_by_matrix4(world2light, i->m_light->rotation().getTransposed());
          matrix4_translate_by_vec3(world2light, -lightBounds.origin); // world->lightBounds
        }

		// Set the ambient factor - 1.0 for an ambient light, 0.0 for normal light
		float ambient = 0.0;
		if (lightShader->isAmbientLight())
			ambient = 1.0;

        current.m_program->setParameters(viewer, *(*i).m_transform, lightBounds.origin + (*i).m_light->offset(), (*i).m_light->colour(), world2light, ambient);
      }
    }

    (*i).m_renderable->render(current.m_state);
  }
  glPopMatrix();
  m_renderables.clear();
}



