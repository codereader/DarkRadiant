#pragma once

#include "igl.h"
#include "iglprogram.h"
#include "ishaders.h"
#include "ishaderlayer.h"
#include "irender.h"

#include "debugging/gl.h"
#include "render/Colour4.h"

namespace render
{

/**
 * \brief
 * Data structure encapsulating various parameters of the OpenGL state machine,
 * as well as parameters used internally by Radiant.
 *
 * The OpenGLState class is used to keep track of OpenGL state parameters used
 * by the renderer, in order to avoid using slow glGet() calls or repeatedly
 * changing states to the same value. Each shader pass keeps an OpenGLState
 * member which stores the state values it wishes to use, and these values are
 * selectively applied to a single "current" OpenGLState object maintained by
 * the render system.
 */
class OpenGLState
{
public:

    /// Enum of possible sort positions
    enum SortPosition
    {
        SORT_FIRST = -64,
        SORT_ZFILL = 0,             // used by depth buffer fill passes
        SORT_INTERACTION = 2,       // used by the DBS pass
        SORT_BACKGROUND = 512,      // used by inactive nodes
        SORT_FULLBRIGHT = 1025,     // used by non-translucent editor passes
        SORT_TRANSLUCENT = 1026,    // used by blend-type editor passes
        SORT_OVERLAY_FIRST = 1028,  // used by decals
        SORT_OVERLAY_SECOND = 1029,  // used by merge actions
        SORT_OVERLAY_THIRD = 1030,   // used by merge actions
        SORT_OVERLAY_ONE_BEFORE_LAST = 2046, // used by merge actions
        SORT_OVERLAY_LAST = 2047,
        SORT_HIGHLIGHT = 2048,      // used by the (red) selection system overlay
        SORT_POINT_FIRST = 2049,    // used by POINT renderers
        SORT_POINT_LAST = 3071,
        SORT_GUI0 = 3072,           // used by selection system controls, pivots (visible)
        SORT_GUI1 = 3073,           // used by selection system controls, pivots (obscured)
        SORT_LAST = 4096,
    };

private:

    // The 4 colour components, only for use in OpenGLStates that do not have
    // any shader stages attached, otherwise pull the colour from there.
    Colour4 _colour;

    // Colour inversion flag
    IShaderLayer::VertexColourMode _vertexColourMode;

    // Set of RENDER_XXX flags
    unsigned _renderFlags;

    // GL depth function
    GLenum _glDepthFunc;

    // Sort position
    SortPosition _sortPos;

	std::string _name;

public:
	const std::string& getName() const
	{
		return _name;
	}

	void setName(const std::string& name)
	{
		_name = name;
	}

    /// Return the glColor for this state
    const Colour4& getColour() const
    {
        assert(_colour.isValid());
        return _colour;
    }

    /// Set the glColor for this state, from a Vector4
    void setColour(const Colour4& col)
    {
        assert(col.isValid());
        _colour = col;
    }

    /// Set the glColor for this state, from individual components
    void setColour(float r, float g, float b, float a)
    {
        setColour(Colour4(r, g, b, a));
    }

    IShaderLayer::VertexColourMode getVertexColourMode() const
	{
        return _vertexColourMode;
	}

    void setVertexColourMode(IShaderLayer::VertexColourMode mode)
	{
        _vertexColourMode = mode;
	}

    /// \name Render flag operations
    ///@{

    /// Set all render flags for this state
    void setRenderFlags(unsigned newFlags) { _renderFlags = newFlags; }

    /// Set a single render flag on this state
    void setRenderFlag(unsigned flag)
    {
        setRenderFlags(_renderFlags | flag);
    }

    /// Clear a single render flag on this state
    void clearRenderFlag(unsigned flag)
    {
        setRenderFlags(_renderFlags & ~flag);
    }

    /// Test the value of a single render flag
    bool testRenderFlag(unsigned flag) const
    {
        return (_renderFlags & flag) > 0;
    }

    /// Return the render flags for this state
    unsigned getRenderFlags() const { return _renderFlags; }

    ///@}

    /// Return the depth function
    GLenum getDepthFunc() const { return _glDepthFunc; }

    /// Set the depth function
    void setDepthFunc(GLenum func) { _glDepthFunc = func; }

    /// Return the sort position
    SortPosition getSortPosition() const { return _sortPos; }

    /// Set the sort position
    void setSortPosition(SortPosition pos) { _sortPos = pos; }

    /**
     * \brief
     * Polygon offset.
     */
    float polygonOffset;

    /**
     * \brief
     * GL texture numbers to be bound to texture units.
     *
     * \{
     */

    GLuint texture0; // diffuse
    GLuint texture1; // bump
    GLuint texture2; // specular
    GLuint texture3; // light texture
    GLuint texture4; // light falloff
    GLuint texture5; // shadow map

    /**
     * \}
     */

    /**
     * Each open GL State refers to one or more shader stages,
     * which hold the actual values of many parameters, some of them
     * time-dependent or depending on entity parameters.
     */
    IShaderLayer::Ptr stage0;
    IShaderLayer::Ptr stage1;
    IShaderLayer::Ptr stage2;
    IShaderLayer::Ptr stage3;
    IShaderLayer::Ptr stage4;

    /**
     * \brief
     * Source blend mode.
     */
    GLenum m_blend_src;

    /**
     * \brief
     * Destination blend mode
     */
    GLenum m_blend_dst;

    // Alpha test function
    GLenum alphaFunc;

    // Alpha test threshold
    GLfloat alphaThreshold;

    GLfloat m_linewidth;
    GLfloat m_pointsize;
    GLint m_linestipple_factor;
    GLushort m_linestipple_pattern;

    /**
     * \brief
     * GL program or shader object.
     */
    GLProgram* glProgram;

    /**
     * \brief
     * The cube-map texgen mode for rendering.
     */
    IShaderLayer::CubeMapMode cubeMapMode;

    // Whether to ignore the RGBA colour modulation defined by the associated shader stage
    // in which case only the _colour member will be used.
    bool ignoreStageColour;

    /// Default constructor
    OpenGLState()
    : _colour(Colour4::WHITE()),
      _vertexColourMode(IShaderLayer::VERTEX_COLOUR_NONE),
      _renderFlags(0),
      _glDepthFunc(GL_LESS),
      _sortPos(SORT_FIRST),
      polygonOffset(0.0f),
      texture0(0),
      texture1(0),
      texture2(0),
      texture3(0),
      texture4(0),
      texture5(0),
      m_blend_src(GL_SRC_ALPHA),
      m_blend_dst(GL_ONE_MINUS_SRC_ALPHA),
      alphaFunc(GL_ALWAYS),
      alphaThreshold(0),
      m_linewidth(1),
      m_pointsize(1),
      m_linestipple_factor(1),
      m_linestipple_pattern(0xAAAA),
      glProgram(nullptr),
      cubeMapMode(IShaderLayer::CUBE_MAP_NONE),
      ignoreStageColour(false)
    { }

    // Determines the difference between this state and the target (current) state.
    // Issues the state calls required by this state and updates the target state
    // to reflect the changes.
    // The given globalStateMask controls which state changes are allowed in the first place.
    void applyTo(OpenGLState& current, unsigned int globalStateMask)
    {
        // Apply the global state mask to our own desired render flags to determine
        // the final set of flags that allow and require changing
        const unsigned requiredState = _renderFlags & globalStateMask;

        // Construct a mask containing all the flags that will be changing between
        // the current state and the required state. This avoids performing
        // unnecessary GL calls to set the state to its existing value.
        const unsigned changingBitsMask = requiredState ^ current.getRenderFlags();

        // Set the GLProgram if required
        if (requiredState & RENDER_PROGRAM)
        {
            activateShaderProgram(current);
        }
        else
        {
            deactivateShaderProgram(current);
        }

        // State changes. Only perform these if changingBitsMask > 0, since if there are
        // no changes required we don't want a whole load of unnecessary bit operations.
        if (changingBitsMask != 0)
        {
            if (changingBitsMask & requiredState & RENDER_FILL)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                debug::assertNoGlErrors();
            }
            else if (changingBitsMask & ~requiredState & RENDER_FILL)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                debug::assertNoGlErrors();
            }

            setState(requiredState, changingBitsMask, RENDER_OFFSETLINE, GL_POLYGON_OFFSET_LINE);

            if (changingBitsMask & requiredState & RENDER_LIGHTING)
            {
                glEnable(GL_LIGHTING);
                glEnable(GL_COLOR_MATERIAL);
                glEnableClientState(GL_NORMAL_ARRAY);
                debug::assertNoGlErrors();
            }
            else if (changingBitsMask & ~requiredState & RENDER_LIGHTING)
            {
                glDisable(GL_LIGHTING);
                glDisable(GL_COLOR_MATERIAL);
                glDisableClientState(GL_NORMAL_ARRAY);
                debug::assertNoGlErrors();
            }

            // RENDER_TEXTURE_CUBEMAP
            if (changingBitsMask & requiredState & RENDER_TEXTURE_CUBEMAP)
            {
                setTexture0();
                glEnable(GL_TEXTURE_CUBE_MAP);
                debug::assertNoGlErrors();
            }
            else if (changingBitsMask & ~requiredState & RENDER_TEXTURE_CUBEMAP)
            {
                setTexture0();
                glDisable(GL_TEXTURE_CUBE_MAP);
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                debug::assertNoGlErrors();
            }

            // RENDER_TEXTURE_2D
            if (changingBitsMask & requiredState & RENDER_TEXTURE_2D)
            {
                setTexture0();
                glEnable(GL_TEXTURE_2D);
                debug::assertNoGlErrors();
            }
            else if (changingBitsMask & ~requiredState & RENDER_TEXTURE_2D)
            {
                setTexture0();
                glDisable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
                debug::assertNoGlErrors();
            }

            // RENDER_BLEND
            if (changingBitsMask & requiredState & RENDER_BLEND)
            {
                glEnable(GL_BLEND);
                setTexture0();
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                debug::assertNoGlErrors();
            }
            else if (changingBitsMask & ~requiredState & RENDER_BLEND)
            {
                glDisable(GL_BLEND);
                setTexture0();
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                debug::assertNoGlErrors();
            }

            setState(requiredState, changingBitsMask, RENDER_CULLFACE, GL_CULL_FACE);

            if (changingBitsMask & requiredState & RENDER_SMOOTH)
            {
                glShadeModel(GL_SMOOTH);
                debug::assertNoGlErrors();
            }
            else if (changingBitsMask & ~requiredState & RENDER_SMOOTH)
            {
                glShadeModel(GL_FLAT);
                debug::assertNoGlErrors();
            }

            setState(requiredState, changingBitsMask, RENDER_SCALED, GL_NORMALIZE); // not GL_RESCALE_NORMAL
            setState(requiredState, changingBitsMask, RENDER_DEPTHTEST, GL_DEPTH_TEST);

            if (changingBitsMask & requiredState & RENDER_DEPTHWRITE)
            {
                glDepthMask(GL_TRUE);
                debug::assertNoGlErrors();
            }
            else if (changingBitsMask & ~requiredState & RENDER_DEPTHWRITE)
            {
                glDepthMask(GL_FALSE);
                debug::assertNoGlErrors();
            }

            // Disable colour buffer writes if required
            if (changingBitsMask & requiredState & RENDER_MASKCOLOUR)
            {
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                debug::assertNoGlErrors();
            }
            else if (changingBitsMask & ~requiredState & RENDER_MASKCOLOUR)
            {
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                debug::assertNoGlErrors();
            }

            setState(requiredState, changingBitsMask, RENDER_ALPHATEST, GL_ALPHA_TEST);

            // Set GL states corresponding to RENDER_ flags
            setState(requiredState, changingBitsMask, RENDER_LINESTIPPLE, GL_LINE_STIPPLE);
            setState(requiredState, changingBitsMask, RENDER_POLYGONSTIPPLE, GL_POLYGON_STIPPLE);

        } // end of changingBitsMask-dependent changes

        // Set depth function
        if (requiredState & RENDER_DEPTHTEST && getDepthFunc() != current.getDepthFunc())
        {
            glDepthFunc(getDepthFunc());
            debug::assertNoGlErrors();
            current.setDepthFunc(getDepthFunc());
        }

        if (requiredState & RENDER_LINESTIPPLE
            && (m_linestipple_factor != current.m_linestipple_factor
                || m_linestipple_pattern != current.m_linestipple_pattern))
        {
            glLineStipple(m_linestipple_factor, m_linestipple_pattern);
            debug::assertNoGlErrors();
            current.m_linestipple_factor = m_linestipple_factor;
            current.m_linestipple_pattern = m_linestipple_pattern;
        }

        // Set up the alpha test parameters
        if (requiredState & RENDER_ALPHATEST && 
            (alphaFunc != current.alphaFunc || alphaThreshold != current.alphaThreshold))
        {
            // Set alpha function in GL
            glAlphaFunc(alphaFunc, alphaThreshold);
            debug::assertNoGlErrors();

            // Store state values
            current.alphaFunc = alphaFunc;
            current.alphaThreshold = alphaThreshold;
        }

        // Apply polygon offset
        if (polygonOffset != current.polygonOffset)
        {
            current.polygonOffset = polygonOffset;

            if (current.polygonOffset > 0.0f)
            {
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(-1, -1 * polygonOffset);
            }
            else
            {
                glDisable(GL_POLYGON_OFFSET_FILL);
            }
        }

        // Apply the GL textures
        applyAllTextures(current, requiredState);

        // Set the GL colour. Do this unconditionally, since setting glColor is
        // cheap and it avoids problems with leaked colour states.
        if (stage0 && !ignoreStageColour)
        {
            setColour(stage0->getColour());
        }
        glColor4fv(getColour());
        current.setColour(getColour());
        debug::assertNoGlErrors();

        if (requiredState & RENDER_BLEND
            && (m_blend_src != current.m_blend_src || m_blend_dst != current.m_blend_dst))
        {
            glBlendFunc(m_blend_src, m_blend_dst);
            debug::assertNoGlErrors();
            current.m_blend_src = m_blend_src;
            current.m_blend_dst = m_blend_dst;
        }

        if (!(requiredState & RENDER_FILL) && m_linewidth != current.m_linewidth)
        {
            glLineWidth(m_linewidth);
            debug::assertNoGlErrors();
            current.m_linewidth = m_linewidth;
        }

        if (!(requiredState & RENDER_FILL) && m_pointsize != current.m_pointsize)
        {
            glPointSize(m_pointsize);
            debug::assertNoGlErrors();
            current.m_pointsize = m_pointsize;
        }

        // Propagate the invert vertex colour flag
        if (requiredState & RENDER_VERTEX_COLOUR)
        {
            current.setVertexColourMode(getVertexColourMode());
        }
        else
        {
            current.setVertexColourMode(IShaderLayer::VERTEX_COLOUR_NONE);
        }

        current.setRenderFlags(requiredState);

        debug::assertNoGlErrors();
    }

    // Bind the given texture to the texture unit, if it is different from the
    // current state, then set the current state to the new texture.
    static void SetTextureState(GLuint& current, const GLuint texture, GLenum textureUnit, GLenum textureMode)
    {
        if (texture == current) return;

        glActiveTexture(textureUnit);
        glClientActiveTexture(textureUnit);
        glBindTexture(textureMode, texture);
        debug::assertNoGlErrors();
        current = texture;
    }

private:
    void setupTextureMatrix(GLenum textureUnit, const IShaderLayer::Ptr& stage)
    {
        // Set the texture matrix for the given unit
        glActiveTexture(textureUnit);
        glClientActiveTexture(textureUnit);

        if (!stage)
        {
            glLoadIdentity();
            return;
        }

        auto tex = stage->getTextureTransform();
        glLoadMatrixd(tex);
    }

    void setTextureState(GLuint& current, const GLuint texture, GLenum textureMode)
    {
        if (texture == current) return;

        glBindTexture(textureMode, texture);
        debug::assertNoGlErrors();
        current = texture;
    }

    // Apply all textures to texture units
    void applyAllTextures(OpenGLState& current, unsigned requiredState)
    {
        // Set the texture dimensionality from render flags. There is only a global
        // mode for all textures, we can't have texture1 as 2D and texture2 as
        // CUBE_MAP for example.
        GLenum textureMode = 0;

        if (requiredState & RENDER_TEXTURE_CUBEMAP) // cube map has priority
        {
            textureMode = GL_TEXTURE_CUBE_MAP;
        }
        else if (requiredState & RENDER_TEXTURE_2D)
        {
            textureMode = GL_TEXTURE_2D;
        }

        // Apply our texture numbers to the current state
        if (textureMode != 0) // only if one of the RENDER_TEXTURE options
        {
            glMatrixMode(GL_TEXTURE);

            if (GLEW_VERSION_1_3)
            {
                SetTextureState(current.texture0, texture0, GL_TEXTURE0, textureMode);
                setupTextureMatrix(GL_TEXTURE0, stage0);

                SetTextureState(current.texture1, texture1, GL_TEXTURE1, textureMode);
                setupTextureMatrix(GL_TEXTURE1, stage1);

                SetTextureState(current.texture2, texture2, GL_TEXTURE2, textureMode);
                setupTextureMatrix(GL_TEXTURE2, stage2);

                SetTextureState(current.texture3, texture2, GL_TEXTURE2, textureMode);
                SetTextureState(current.texture4, texture2, GL_TEXTURE2, textureMode);

                glActiveTexture(GL_TEXTURE0);
                glClientActiveTexture(GL_TEXTURE0);
            }
            else
            {
                setTextureState(current.texture0, texture0, textureMode);
                setupTextureMatrix(GL_TEXTURE0, stage0);
            }

            glMatrixMode(GL_MODELVIEW);
        }
    }

    void setTexture0()
    {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }

    // Utility function to toggle an OpenGL state flag
    void setState(unsigned int state, unsigned int delta, unsigned int flag, GLenum glflag)
    {
        if (delta & state & flag)
        {
            glEnable(glflag);
            debug::assertNoGlErrors();
        }
        else if (delta & ~state & flag)
        {
            glDisable(glflag);
            debug::assertNoGlErrors();
        }
    }

    void activateShaderProgram(OpenGLState& current)
    {
        if (current.glProgram == glProgram)
        {
            // nothing to do
            return;
        }

        // Deactivate the previous program first
        deactivateShaderProgram(current);

        if (glProgram != nullptr)
        {
            current.glProgram = glProgram;
            current.glProgram->enable();
        }
    }

    void deactivateShaderProgram(OpenGLState& current)
    {
        if (current.glProgram == nullptr) return;

        current.glProgram->disable();
        current.glProgram = nullptr;
    }
};

}
