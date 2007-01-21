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

// Another texture state setter
inline void setTextureState(GLint& current, const GLint& texture)
{
  if(texture != current)
  {
    glBindTexture(GL_TEXTURE_2D, texture);
    GlobalOpenGL_debugAssertNoErrors();
    current = texture;
  }
}



// Utility function to toggle an OpenGL state flag
inline void setState(unsigned int state, 
					 unsigned int delta, 
					 unsigned int flag, 
					 GLenum glflag)
{
  if(delta & state & flag)
  {
    glEnable(glflag);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & flag)
  {
    glDisable(glflag);
    GlobalOpenGL_debugAssertNoErrors();
  }
}

} // namespace

// Apply own state to current state object
void OpenGLStateBucket::applyState(OpenGLState& current,
								   unsigned int globalstate) 
{
  if(_state.m_state & RENDER_OVERRIDE)
  {
    globalstate |= RENDER_FILL | RENDER_DEPTHWRITE;
  }

  const unsigned int state = _state.m_state & globalstate;
  const unsigned int delta = state ^ current.m_state;

  GlobalOpenGL_debugAssertNoErrors();

  GLProgram* program = (state & RENDER_PROGRAM) != 0 ? _state.m_program : 0;

  if(program != current.m_program)
  {
    if(current.m_program != 0)
    {
      current.m_program->disable();
      glColor4fv(current.m_colour);
    }

    current.m_program = program;

    if(current.m_program != 0)
    {
      current.m_program->enable();
    }
  }

  if(delta & state & RENDER_FILL)
  {
    //qglPolygonMode (GL_BACK, GL_LINE);
    //qglPolygonMode (GL_FRONT, GL_FILL);
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_FILL)
  {
    glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_OFFSETLINE, GL_POLYGON_OFFSET_LINE);

  if(delta & state & RENDER_LIGHTING)
  {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    //qglEnable(GL_RESCALE_NORMAL);
    glEnableClientState(GL_NORMAL_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_LIGHTING)
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    //qglDisable(GL_RESCALE_NORMAL);
    glDisableClientState(GL_NORMAL_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
  }

  if(delta & state & RENDER_TEXTURE)
  {
    GlobalOpenGL_debugAssertNoErrors();

    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
      glClientActiveTexture(GL_TEXTURE0);
    }

    glEnable(GL_TEXTURE_2D);

    glColor4f(1,1,1,_state.m_colour[3]);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_TEXTURE)
  {
    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
      glClientActiveTexture(GL_TEXTURE0);
    }

    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    GlobalOpenGL_debugAssertNoErrors();
  }

  if(delta & state & RENDER_BLEND)
  {
// FIXME: some .TGA are buggy, have a completely empty alpha channel
// if such brushes are rendered in this loop they would be totally transparent with GL_MODULATE
// so I decided using GL_DECAL instead
// if an empty-alpha-channel or nearly-empty texture is used. It will be blank-transparent.
// this could get better if you can get glTexEnviv (GL_TEXTURE_ENV, to work .. patches are welcome

    glEnable(GL_BLEND);
    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_BLEND)
  {
    glDisable(GL_BLEND);
    if(GLEW_VERSION_1_3)
    {
      glActiveTexture(GL_TEXTURE0);
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_CULLFACE, GL_CULL_FACE);

  if(delta & state & RENDER_SMOOTH)
  {
    glShadeModel(GL_SMOOTH);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_SMOOTH)
  {
    glShadeModel(GL_FLAT);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_SCALED, GL_NORMALIZE); // not GL_RESCALE_NORMAL

  setState(state, delta, RENDER_DEPTHTEST, GL_DEPTH_TEST);

  if(delta & state & RENDER_DEPTHWRITE)
  {
    glDepthMask(GL_TRUE);

    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_DEPTHWRITE)
  {
    glDepthMask(GL_FALSE);

    GlobalOpenGL_debugAssertNoErrors();
  }

  if(delta & state & RENDER_COLOURWRITE)
  {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_COLOURWRITE)
  {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_ALPHATEST, GL_ALPHA_TEST);
  
  if(delta & state & RENDER_COLOURARRAY)
  {
    glEnableClientState(GL_COLOR_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
  }
  else if(delta & ~state & RENDER_COLOURARRAY)
  {
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4fv(_state.m_colour);
    GlobalOpenGL_debugAssertNoErrors();
  }

  if(delta & ~state & RENDER_COLOURCHANGE)
  {
    glColor4fv(_state.m_colour);
    GlobalOpenGL_debugAssertNoErrors();
  }

  setState(state, delta, RENDER_LINESTIPPLE, GL_LINE_STIPPLE);
  setState(state, delta, RENDER_LINESMOOTH, GL_LINE_SMOOTH);

  setState(state, delta, RENDER_POLYGONSTIPPLE, GL_POLYGON_STIPPLE);
  setState(state, delta, RENDER_POLYGONSMOOTH, GL_POLYGON_SMOOTH);

  setState(state, delta, RENDER_FOG, GL_FOG);

  if(state & RENDER_DEPTHTEST && _state.m_depthfunc != current.m_depthfunc)
  {
    glDepthFunc(_state.m_depthfunc);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_depthfunc = _state.m_depthfunc;
  }

  if(state & RENDER_LINESTIPPLE
    && (_state.m_linestipple_factor != current.m_linestipple_factor
    || _state.m_linestipple_pattern != current.m_linestipple_pattern))
  {
    glLineStipple(_state.m_linestipple_factor, _state.m_linestipple_pattern);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_linestipple_factor = _state.m_linestipple_factor;
    current.m_linestipple_pattern = _state.m_linestipple_pattern;
  }


  if(state & RENDER_ALPHATEST
    && ( _state.m_alphafunc != current.m_alphafunc
    || _state.m_alpharef != current.m_alpharef ) )
  {
    glAlphaFunc(_state.m_alphafunc, _state.m_alpharef);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_alphafunc = _state.m_alphafunc;
    current.m_alpharef = _state.m_alpharef;
  }

  {
    GLint texture0 = 0;
    GLint texture1 = 0;
    GLint texture2 = 0;
    GLint texture3 = 0;
    GLint texture4 = 0;
    GLint texture5 = 0;
    GLint texture6 = 0;
    GLint texture7 = 0;
    //if(state & RENDER_TEXTURE) != 0)
    {
      texture0 = _state.m_texture;
      texture1 = _state.m_texture1;
      texture2 = _state.m_texture2;
      texture3 = _state.m_texture3;
      texture4 = _state.m_texture4;
      texture5 = _state.m_texture5;
      texture6 = _state.m_texture6;
      texture7 = _state.m_texture7;
    }

    if(GLEW_VERSION_1_3)
    {
      setTextureState(current.m_texture, texture0, GL_TEXTURE0);
      setTextureState(current.m_texture1, texture1, GL_TEXTURE1);
      setTextureState(current.m_texture2, texture2, GL_TEXTURE2);
      setTextureState(current.m_texture3, texture3, GL_TEXTURE3);
      setTextureState(current.m_texture4, texture4, GL_TEXTURE4);
      setTextureState(current.m_texture5, texture5, GL_TEXTURE5);
      setTextureState(current.m_texture6, texture6, GL_TEXTURE6);
      setTextureState(current.m_texture7, texture7, GL_TEXTURE7);
    }
    else
    {
      setTextureState(current.m_texture, texture0);
    }
  }


  if(state & RENDER_TEXTURE && _state.m_colour[3] != current.m_colour[3])
  {
    glColor4f(1,1,1,_state.m_colour[3]);
    GlobalOpenGL_debugAssertNoErrors();
  }

  if(!(state & RENDER_TEXTURE)
    && (_state.m_colour[0] != current.m_colour[0]
    || _state.m_colour[1] != current.m_colour[1]
    || _state.m_colour[2] != current.m_colour[2]
    || _state.m_colour[3] != current.m_colour[3]))
  {
    glColor4fv(_state.m_colour);
    GlobalOpenGL_debugAssertNoErrors();
  }
  current.m_colour = _state.m_colour;

  if(state & RENDER_BLEND
    && (_state.m_blend_src != current.m_blend_src || _state.m_blend_dst != current.m_blend_dst))
  {
    glBlendFunc(_state.m_blend_src, _state.m_blend_dst);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_blend_src = _state.m_blend_src;
    current.m_blend_dst = _state.m_blend_dst;
  }

  if(!(state & RENDER_FILL)
    && _state.m_linewidth != current.m_linewidth)
  {
    glLineWidth(_state.m_linewidth);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_linewidth = _state.m_linewidth;
  }

  if(!(state & RENDER_FILL)
    && _state.m_pointsize != current.m_pointsize)
  {
    glPointSize(_state.m_pointsize);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_pointsize = _state.m_pointsize;
  }

  current.m_state = state;

  GlobalOpenGL_debugAssertNoErrors();
}

// Render the bucket contents
void OpenGLStateBucket::render(OpenGLState& current, 
							   unsigned int globalstate, 
							   const Vector3& viewer)
{
	// Apply our state to the current state object
	applyState(current, globalstate);
	
  if((globalstate & _state.m_state & RENDER_SCREEN) != 0)
  {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(g_matrix4_identity);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(g_matrix4_identity);

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
	else if(!m_renderables.empty()) {
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



