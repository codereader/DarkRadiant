#include "OpenGLShaderPass.h"
#include "OpenGLShader.h"

#include "math/matrix.h"
#include "math/aabb.h"
#include "irender.h"
#include "ishaders.h"
#include "texturelib.h"

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

// GL state enabling/disabling helpers

void OpenGLShaderPass::enableRenderTexture()
{
    GlobalOpenGL_debugAssertNoErrors();

    if(GLEW_VERSION_1_3)
    {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }

    glEnable(GL_TEXTURE_2D);

    //glColor4d(1,1,1,_state.m_colour[3]);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
}

void OpenGLShaderPass::disableRenderTexture()
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

void OpenGLShaderPass::enableRenderBlend()
{
    glEnable(GL_BLEND);
    if(GLEW_VERSION_1_3)
    {
        glActiveTexture(GL_TEXTURE0);
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL_debugAssertNoErrors();
}

void OpenGLShaderPass::disableRenderBlend()
{
    glDisable(GL_BLEND);
    if(GLEW_VERSION_1_3)
    {
        glActiveTexture(GL_TEXTURE0);
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL_debugAssertNoErrors();
}

// Apply own state to current state object
void OpenGLShaderPass::applyState(OpenGLState& current,
					              unsigned int globalStateMask) 
{
  if(_state.renderFlags & RENDER_OVERRIDE)
  {
    globalStateMask |= RENDER_FILL | RENDER_DEPTHWRITE;
  }
  
    // Apply the global state mask to our own desired render flags to determine
    // the final set of flags that must bet set
	const unsigned int requiredState = _state.renderFlags & globalStateMask;
	
    // Construct a mask containing all the flags that will be changing between
    // the current state and the required state. This avoids performing
    // unnecessary GL calls to set the state to its existing value.
	const unsigned int changingBitsMask = requiredState ^ current.renderFlags;

    // Set the GLProgram if required
	GLProgram* program = (requiredState & RENDER_PROGRAM) != 0 
						  ? _state.m_program 
						  : 0;
						  
    if(program != current.m_program)
    {
        if(current.m_program != 0)
        {
          current.m_program->disable();
          glColor4dv(current.m_colour);
        }

        current.m_program = program;

        if(current.m_program != 0)
        {
          current.m_program->enable();
        }
    }

    // State changes. Only perform these if changingBitsMask > 0, since if there are
    // no changes required we don't want a whole load of unnecessary bit
    // operations.
    if (changingBitsMask != 0) {
        if(changingBitsMask & requiredState & RENDER_FILL)
        {
            glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
            GlobalOpenGL_debugAssertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_FILL)
        {
            glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
            GlobalOpenGL_debugAssertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_OFFSETLINE, GL_POLYGON_OFFSET_LINE);

        if(changingBitsMask & requiredState & RENDER_LIGHTING)
        {
            glEnable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);
            glEnableClientState(GL_NORMAL_ARRAY);
            GlobalOpenGL_debugAssertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_LIGHTING)
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_COLOR_MATERIAL);
            glDisableClientState(GL_NORMAL_ARRAY);
            GlobalOpenGL_debugAssertNoErrors();
        }

        // RENDER_TEXTURE_2D
        if(changingBitsMask & requiredState & RENDER_TEXTURE_2D)
        { 
            enableRenderTexture();
        }
        else if(changingBitsMask & ~requiredState & RENDER_TEXTURE_2D)
        { 
            disableRenderTexture();
        }

        // RENDER_BLEND
        if(changingBitsMask & requiredState & RENDER_BLEND)
        { 
            enableRenderBlend();
        }
        else if(changingBitsMask & ~requiredState & RENDER_BLEND)
        { 
            disableRenderBlend();
        }

        setState(requiredState, changingBitsMask, RENDER_CULLFACE, GL_CULL_FACE);

        if(changingBitsMask & requiredState & RENDER_SMOOTH)
        {
            glShadeModel(GL_SMOOTH);
            GlobalOpenGL_debugAssertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_SMOOTH)
        {
            glShadeModel(GL_FLAT);
            GlobalOpenGL_debugAssertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_SCALED, GL_NORMALIZE); // not GL_RESCALE_NORMAL

        setState(requiredState, changingBitsMask, RENDER_DEPTHTEST, GL_DEPTH_TEST);

        if(changingBitsMask & requiredState & RENDER_DEPTHWRITE)
        {
            glDepthMask(GL_TRUE);

            GlobalOpenGL_debugAssertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_DEPTHWRITE)
        {
            glDepthMask(GL_FALSE);

            GlobalOpenGL_debugAssertNoErrors();
        }

        if(changingBitsMask & requiredState & RENDER_COLOURWRITE)
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            GlobalOpenGL_debugAssertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_COLOURWRITE)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            GlobalOpenGL_debugAssertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_ALPHATEST, GL_ALPHA_TEST);

        if(changingBitsMask & requiredState & RENDER_COLOURARRAY)
        {
            glEnableClientState(GL_COLOR_ARRAY);
            GlobalOpenGL_debugAssertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_COLOURARRAY)
        {
            glDisableClientState(GL_COLOR_ARRAY);
            glColor4dv(_state.m_colour);
            GlobalOpenGL_debugAssertNoErrors();
        }

        if(changingBitsMask & ~requiredState & RENDER_COLOURCHANGE)
        {
            glColor4dv(_state.m_colour);
            GlobalOpenGL_debugAssertNoErrors();
        }

        // Set GL states corresponding to RENDER_ flags
        setState(requiredState, changingBitsMask, RENDER_LINESTIPPLE, GL_LINE_STIPPLE);
        setState(requiredState, changingBitsMask, RENDER_LINESMOOTH, GL_LINE_SMOOTH);

        setState(requiredState, changingBitsMask, RENDER_POLYGONSTIPPLE, GL_POLYGON_STIPPLE);
        setState(requiredState, changingBitsMask, RENDER_POLYGONSMOOTH, GL_POLYGON_SMOOTH);

    } // end of changingBitsMask-dependent changes

  if(requiredState & RENDER_DEPTHTEST && _state.m_depthfunc != current.m_depthfunc)
  {
    glDepthFunc(_state.m_depthfunc);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_depthfunc = _state.m_depthfunc;
  }

  if(requiredState & RENDER_LINESTIPPLE
    && (_state.m_linestipple_factor != current.m_linestipple_factor
    || _state.m_linestipple_pattern != current.m_linestipple_pattern))
  {
    glLineStipple(_state.m_linestipple_factor, _state.m_linestipple_pattern);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_linestipple_factor = _state.m_linestipple_factor;
    current.m_linestipple_pattern = _state.m_linestipple_pattern;
  }


  if(requiredState & RENDER_ALPHATEST
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
    //if(state & RENDER_TEXTURE_2D) != 0)
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


#if 0
  if(requiredState & RENDER_TEXTURE_2D && _state.m_colour[3] != current.m_colour[3])
  {
    glColor4d(1,1,1,_state.m_colour[3]);
    GlobalOpenGL_debugAssertNoErrors();
  }
#endif

    // Set the GL colour if it isn't set already
    if (_state.m_colour != current.m_colour)
    {
        glColor4dv(_state.m_colour);
        current.m_colour = _state.m_colour;
        GlobalOpenGL_debugAssertNoErrors();
    }

  if(requiredState & RENDER_BLEND
    && (_state.m_blend_src != current.m_blend_src || _state.m_blend_dst != current.m_blend_dst))
  {
    glBlendFunc(_state.m_blend_src, _state.m_blend_dst);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_blend_src = _state.m_blend_src;
    current.m_blend_dst = _state.m_blend_dst;
  }

  if(!(requiredState & RENDER_FILL)
    && _state.m_linewidth != current.m_linewidth)
  {
    glLineWidth(_state.m_linewidth);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_linewidth = _state.m_linewidth;
  }

  if(!(requiredState & RENDER_FILL)
    && _state.m_pointsize != current.m_pointsize)
  {
    glPointSize(_state.m_pointsize);
    GlobalOpenGL_debugAssertNoErrors();
    current.m_pointsize = _state.m_pointsize;
  }

  current.renderFlags = requiredState;

  GlobalOpenGL_debugAssertNoErrors();
}

// DEBUG: Stream insertion for RendererLight 

#include "math/aabb.h"

inline 
std::ostream& operator<< (std::ostream& os, const RendererLight& light) {
	os << "RendererLight { aabb = " << light.aabb()
	   << ", offset = " << light.offset() << ", colour = " << light.colour()
	   << " }";
	return os;
}

// Add a Renderable to this bucket
void OpenGLShaderPass::addRenderable(const OpenGLRenderable& renderable, 
									  const Matrix4& modelview, 
									  const RendererLight* light)
{
	_renderables.push_back(TransformedRenderable(renderable, modelview, light));
}


// Render the bucket contents
void OpenGLShaderPass::render(OpenGLState& current, 
							   unsigned int globalstate, 
							   const Vector3& viewer)
{
	// Apply our state to the current state object
	applyState(current, globalstate);
	
  if((globalstate & _state.renderFlags & RENDER_SCREEN) != 0)
  {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(Matrix4::getIdentity());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(Matrix4::getIdentity());

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
	else if(!_renderables.empty()) {
		flushRenderables(current, globalstate, viewer);
	}
}

// Flush renderables
void OpenGLShaderPass::flushRenderables(OpenGLState& current, 
										 unsigned int globalstate, 
										 const Vector3& viewer)
{
	// Keep a pointer to the last transform matrix used
	const Matrix4* transform = 0;
	
	glPushMatrix();
	
	// Iterate over each transformed renderable in the vector
	for(OpenGLShaderPass::Renderables::const_iterator i = _renderables.begin(); 
  	  	i != _renderables.end(); 
  	  	++i)
	{
		// If the current iteration's transform matrix was different from the
		// last, apply it and store for the next iteration
	    if (!transform 
	    	|| (transform != i->transform 
	    		&& !matrix4_affine_equal(*transform, *(*i).transform)))
		{
			transform = i->transform;
      		glPopMatrix();
      		glPushMatrix();
      		glMultMatrixd(*transform);
      		
      		// Determine the face direction
      		if ((current.renderFlags & RENDER_CULLFACE) != 0
      			&& matrix4_handedness(*transform) == MATRIX4_RIGHTHANDED)
      		{
      			glFrontFace(GL_CW);
      		}
      		else 
      		{
      			glFrontFace(GL_CCW);
      		}
    	}

		// If we are using a lighting program and this renderable is lit, set
		// up the lighting calculation
		const RendererLight* light = i->light;

		if (current.m_program != 0 && light != NULL) 
        {
			// Get the light shader and examine its first (and only valid) layer
			IShaderPtr lightShader = light->getShader()->getIShader();
      
			if (lightShader->firstLayer() != 0) 
            {
				// Get the XY and Z falloff texture numbers.
	        	GLuint attenuation_xy = 
	        		lightShader->firstLayer()->getTexture()->getGLTexNum();
                GLuint attenuation_z = 
                	lightShader->lightFalloffImage()->getGLTexNum();

                // Bind the falloff textures
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

                // Calculate the world-space to light-space transformation
                // matrix
                AABB lightBounds(light->aabb());

                Matrix4 world2light(Matrix4::getIdentity());

                if (light->isProjected()) 
                {
                  world2light = light->projection();
                  matrix4_multiply_by_matrix4(world2light, light->rotation().getTransposed());
                  
                  // greebo: old code: matrix4_translate_by_vec3(world2light, -lightBounds.origin); // world->lightBounds
                  matrix4_translate_by_vec3(world2light, -(light->offset()));
                }
                else 
                {
                  matrix4_translate_by_vec3(world2light, Vector3(0.5f, 0.5f, 0.5f));
                  matrix4_scale_by_vec3(world2light, Vector3(0.5f, 0.5f, 0.5f));
                  matrix4_scale_by_vec3(world2light, Vector3(1.0f / lightBounds.extents.x(), 1.0f / lightBounds.extents.y(), 1.0f / lightBounds.extents.z()));
                  matrix4_multiply_by_matrix4(world2light, light->rotation().getTransposed());
                  matrix4_translate_by_vec3(world2light, -lightBounds.origin); // world->lightBounds
                }

                // Set the ambient factor - 1.0 for an ambient light, 0.0 for normal light
                float ambient = 0.0;
                if (lightShader->isAmbientLight())
                    ambient = 1.0;

                // Bind the GL program parameters
                Vector3 lightOrigin = (light->isProjected() 
                                       ? light->worldOrigin()
                                       : lightBounds.origin + light->offset());
                current.m_program->applyRenderParams(
                    viewer,
                    *i->transform,
                    lightOrigin,
                    light->colour(),
                    world2light,
                    ambient
                );
            }
        }

        // Render the renderable
        i->renderable->render(current.renderFlags);
    }

    // Cleanup
    glPopMatrix();
    _renderables.clear();
}



