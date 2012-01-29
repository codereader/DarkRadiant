#pragma once

#include "igl.h"
#include "imodule.h"
#include "ishaders.h"
#include "ShaderLayer.h"

#include "math/Vector4.h"

#include <vector>

// Full declaration in iglprogram.h
class GLProgram;

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
    // The 4 colour components, only for use in OpenGLStates that do not have
    // any shader stages attached, otherwise pull the colour from there.
	Colour4 _colour;

    // Colour inversion flag
    bool _invertColour;

    // Set of RENDER_XXX flags
    unsigned _renderFlags;

public:
	enum ESort
	{
		eSortFirst = -64,
		eSortOpaque = 0,			// used by depth buffer fill passes
		eSortPortalSky = 1,

		eSortMultiFirst = 2,		// used by the DBS pass
		eSortMultiLast = 1023,

		eSortOverbrighten = 1024,
		eSortFullbright = 1025,		// used by non-translucent editor passes

		eSortTranslucent = 1026,	// used by blend-type editor passes

		eSortHighlight = 1027,		// used by the (red) selection system overlay

		eSortOverlayFirst = 1028,	// used by decals
		eSortOverlayLast = 2047,

		eSortControlFirst = 2048,	// used by POINT renderers
		eSortControlLast = 3071,

		eSortGUI0 = 3072,			// used by selection system controls, pivots (visible)
		eSortGUI1 = 3073,			// used by selection system controls, pivots (obscured)

		eSortLast = 4096,
	};

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

    /// Set this state to invert colour values
    void setColourInverted(bool inverted) { _invertColour = inverted; }

    /// Test whether this state is inverting colour values
    bool isColourInverted() const         { return _invertColour; }

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

    /**
     * \brief
     * Sort position.
     */
    int m_sort;

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

    GLint texture0;
    GLint texture1;
    GLint texture2;
    GLint texture3;
    GLint texture4;

    /**
     * \}
     */

	/**
     * Each open GL State refers to one or more shader stages,
	 * which hold the actual values of many parameters, some of them
	 * time-dependent or depending on entity parameters.
	 */
	ShaderLayerPtr stage0;
	ShaderLayerPtr stage1;
	ShaderLayerPtr stage2;
	ShaderLayerPtr stage3;
	ShaderLayerPtr stage4;

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

	GLenum m_depthfunc;

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
    ShaderLayer::CubeMapMode cubeMapMode;

	/// Default constructor
	OpenGLState()
	: _colour(Colour4::WHITE()),
      _invertColour(false),
	  _renderFlags(0),
	  m_sort(eSortFirst),
      polygonOffset(0.0f),
	  texture0(0),
	  texture1(0),
	  texture2(0),
      texture3(0),
      texture4(0),
	  m_blend_src(GL_SRC_ALPHA),
	  m_blend_dst(GL_ONE_MINUS_SRC_ALPHA),
	  m_depthfunc(GL_LESS),
	  alphaFunc(GL_ALWAYS),
	  alphaThreshold(0),
	  m_linewidth(1),
	  m_pointsize(1),
	  m_linestipple_factor(1),
	  m_linestipple_pattern(0xAAAA),
	  glProgram(NULL),
      cubeMapMode(ShaderLayer::CUBE_MAP_NONE)
	{ }
};
