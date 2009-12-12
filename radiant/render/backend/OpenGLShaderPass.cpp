#include "OpenGLShaderPass.h"
#include "OpenGLShader.h"

#include "math/matrix.h"
#include "math/aabb.h"
#include "irender.h"
#include "ishaders.h"
#include "texturelib.h"

namespace {
	
// Bind the given texture to the texture unit, if it is different from the
// current state, then set the current state to the new texture.
inline void setTextureState(GLint& current, 
							const GLint& texture, 
							GLenum textureUnit,
                            GLenum textureMode)
{
    if(texture != current)
    {
        glActiveTexture(textureUnit);
        glClientActiveTexture(textureUnit);
        glBindTexture(textureMode, texture);
        GlobalOpenGL_debugAssertNoErrors();
        current = texture;
    }
}

// Same as setTextureState() above without texture unit parameter
inline void setTextureState(GLint& current,
                            const GLint& texture,
                            GLenum textureMode)
{
  if(texture != current)
  {
    glBindTexture(textureMode, texture);
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

void OpenGLShaderPass::setTexture0()
{
    if(GLEW_VERSION_1_3)
    {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }
}

void OpenGLShaderPass::enableTexture2D()
{
    GlobalOpenGL_debugAssertNoErrors();

    setTexture0();
    glEnable(GL_TEXTURE_2D);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GlobalOpenGL_debugAssertNoErrors();
}

void OpenGLShaderPass::disableTexture2D()
{
    setTexture0();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    GlobalOpenGL_debugAssertNoErrors();
}

// Enable cubemap texturing and texcoord array
void OpenGLShaderPass::enableTextureCubeMap()
{
    setTexture0();
    glEnable(GL_TEXTURE_CUBE_MAP);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    GlobalOpenGL_debugAssertNoErrors();
}

// Disable cubemap texturing and texcoord array
void OpenGLShaderPass::disableTextureCubeMap()
{
    setTexture0();
    glDisable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    GlobalOpenGL_debugAssertNoErrors();
}

void OpenGLShaderPass::enableRenderBlend()
{
    glEnable(GL_BLEND);
    setTexture0();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL_debugAssertNoErrors();
}

void OpenGLShaderPass::disableRenderBlend()
{
    glDisable(GL_BLEND);
    setTexture0();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL_debugAssertNoErrors();
}

// Apply all textures to texture units
void OpenGLShaderPass::applyAllTextures(OpenGLState& current,
                                        unsigned requiredState)
{
    // Set the texture dimensionality from render flags. There is only a global
    // mode for all textures, we can't have texture1 as 2D and texture2 as
    // CUBE_MAP for example.
    GLenum textureMode = 0;
    if (requiredState & RENDER_TEXTURE_CUBEMAP) // cube map has priority
        textureMode = GL_TEXTURE_CUBE_MAP;
    else if (requiredState & RENDER_TEXTURE_2D)
        textureMode = GL_TEXTURE_2D;

    // Apply our texture numbers to the current state
    if (textureMode != 0) // only if one of the RENDER_TEXTURE options
    {
        if(GLEW_VERSION_1_3)
        {
            setTextureState(
                current.texture0, _state.texture0, GL_TEXTURE0, textureMode
            );
            setTextureState(
                current.texture1, _state.texture1, GL_TEXTURE1, textureMode
            );
            setTextureState(
                current.texture2, _state.texture2, GL_TEXTURE2, textureMode
            );
            setTextureState(
                current.texture3, _state.texture2, GL_TEXTURE2, textureMode
            );
            setTextureState(
                current.texture4, _state.texture2, GL_TEXTURE2, textureMode
            );
        }
        else
        {
            setTextureState(
                current.texture0, _state.texture0, textureMode
            );
        }
    }
}

// Set up cube map
void OpenGLShaderPass::setUpCubeMapAndTexGen(OpenGLState& current,
                                             unsigned requiredState,
                                             const Vector3& viewer)
{
    if (requiredState & RENDER_TEXTURE_CUBEMAP)
    {
        // Copy cubemap mode enum to current state object
        current.cubeMapMode = _state.cubeMapMode;

        // Apply axis transformation (swap Y and Z coordinates)
        Matrix4 transform = Matrix4::byRows(
            1, 0, 0, 0,
            0, 0, 1, 0,
            0, 1, 0, 0,
            0, 0, 0, 1
        );

        // Subtract the viewer position
        transform.translateBy(-viewer);

        // Apply to the texture matrix
        glMatrixMode(GL_TEXTURE);
        glLoadMatrixd(transform);
        glMatrixMode(GL_MODELVIEW);
    }
}

// Apply own state to current state object
void OpenGLShaderPass::applyState(OpenGLState& current,
					              unsigned int globalStateMask,
                                  const Vector3& viewer)
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
    if (changingBitsMask != 0) 
    {
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
            enableTexture2D();
        }
        else if(changingBitsMask & ~requiredState & RENDER_TEXTURE_2D)
        { 
            disableTexture2D();
        }

        // RENDER_TEXTURE_CUBEMAP
        if(changingBitsMask & requiredState & RENDER_TEXTURE_CUBEMAP)
        { 
            enableTextureCubeMap();
        }
        else if(changingBitsMask & ~requiredState & RENDER_TEXTURE_CUBEMAP)
        { 
            disableTextureCubeMap();
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

    // Set up the alpha test parameters
    if (requiredState & RENDER_ALPHATEST
        && ( _state.alphaFunc != current.alphaFunc
            || _state.alphaThreshold != current.alphaThreshold) 
    )
    {
        // Set alpha function in GL
        glAlphaFunc(_state.alphaFunc, _state.alphaThreshold);
        GlobalOpenGL_debugAssertNoErrors();

        // Store state values
        current.alphaFunc = _state.alphaFunc;
        current.alphaThreshold = _state.alphaThreshold;
    }

    // Apply polygon offset
    if (_state.polygonOffset != current.polygonOffset)
    {
        glPolygonOffset(1, -_state.polygonOffset * 128);
        current.polygonOffset = _state.polygonOffset;

        if (current.polygonOffset > 0.0f)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
        }
        else
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    // Apply the GL textures
    applyAllTextures(current, requiredState);

    // Set the GL colour if it isn't set already
    if (_state.m_colour != current.m_colour)
    {
        glColor4dv(_state.m_colour);
        current.m_colour = _state.m_colour;
        GlobalOpenGL_debugAssertNoErrors();
    }

    // Set up the cubemap and texgen parameters
    setUpCubeMapAndTexGen(current, requiredState, viewer);

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
	os << "RendererLight { origin = " << light.worldOrigin()
	   << ", lightOrigin = " << light.getLightOrigin() 
       << ", colour = " << light.colour()
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
                              unsigned int flagsMask, 
                              const Vector3& viewer)
{
    // Reset the texture matrix
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(Matrix4::getIdentity());
    glMatrixMode(GL_MODELVIEW);

	// Apply our state to the current state object
	applyState(current, flagsMask, viewer);
	
    // If RENDER_SCREEN is set, just render a quad, otherwise render all
    // objects.
    if((flagsMask & _state.renderFlags & RENDER_SCREEN) != 0)
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
	else if(!_renderables.empty()) 
    {
		renderAllContained(current, viewer);
	}
}

// Setup lighting
void OpenGLShaderPass::setUpLightingCalculation(OpenGLState& current,
                                                const RendererLight* light,
                                                const Vector3& viewer,
                                                const Matrix4& objTransform)
{
    assert(light);

    // Get the light shader and examine its first (and only valid) layer
    MaterialPtr lightShader = light->getShader()->getMaterial();

    if (lightShader->firstLayer() != 0) 
    {
        // Calculate viewer location in object space
        Matrix4 inverseObjTransform = objTransform.getInverse();
        Vector3 osViewer = matrix4_transformed_point(
                inverseObjTransform, viewer
        );

        // Get the XY and Z falloff texture numbers.
        GLuint attenuation_xy = 
            lightShader->firstLayer()->getTexture()->getGLTexNum();
        GLuint attenuation_z = 
            lightShader->lightFalloffImage()->getGLTexNum();

        // Bind the falloff textures
        assert(current.renderFlags & RENDER_TEXTURE_2D);

        setTextureState(
            current.texture3, attenuation_xy, GL_TEXTURE3, GL_TEXTURE_2D
        );
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        setTextureState(
            current.texture4, attenuation_z, GL_TEXTURE4, GL_TEXTURE_2D
        );
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        // Get the world-space to light-space transformation matrix
        Matrix4 world2light = light->getLightTextureTransformation();

        // Set the ambient factor - 1.0 for an ambient light, 0.0 for normal light
        float ambient = 0.0;
        if (lightShader->isAmbientLight())
            ambient = 1.0;

        // Bind the GL program parameters
        current.m_program->applyRenderParams(
            osViewer,
            objTransform,
            light->getLightOrigin(),
            light->colour(),
            world2light,
            ambient
        );
    }
}

// Flush renderables
void OpenGLShaderPass::renderAllContained(OpenGLState& current,
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
            setUpLightingCalculation(current, light, viewer, *transform);
        }

        // Render the renderable
        RenderInfo info(current.renderFlags, viewer, current.cubeMapMode);
        i->renderable->render(info);
    }

    // Cleanup
    glPopMatrix();
    _renderables.clear();
}



