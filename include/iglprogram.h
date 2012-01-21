#pragma once

#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "render/Colour4.h"

/**
 * Representation of a GL vertex/fragment program.
 */
class GLProgram
{
public:
    /**
	 * Destructor
	 */
	virtual ~GLProgram() {}

	/**
	 * Create this program using glGenProgramsARB and siblings. This needs the
	 * OpenGL system to be initialised so cannot happen in the constructor.
	 */
	virtual void create() = 0;

	/**
	 * Destroy this program using GL calls.
	 */
	virtual void destroy() = 0;

	/**
	 * Bind this program as the currently-operative shader.
	 */
	virtual void enable() = 0;

  	/**
  	 * Unbind this program from OpenGL.
  	 */
	virtual void disable() = 0;

    /// Data structure containing rendering parameters
    struct Params
    {
        /// Light origin in world space
        Vector3 lightOrigin;

        /// Light colour
        Colour4 lightColour;

        /// Transformation from world space into light space
        Matrix4 world2Light;

        /// Amount of directionality, 0.0 for normal light, 1.0 for ambient
        float ambientFactor;

        /// Whether vertex colours should be inverted
        bool invertVertexColour;

        Params(const Vector3& lightOrigin_,
               const Colour4& lightColour_,
               const Matrix4& world2Light_)
        : lightOrigin(lightOrigin_),
          lightColour(lightColour_),
          world2Light(world2Light_),
          ambientFactor(0.0),
          invertVertexColour(false)
        {

        }
    };

	/**
	 * \brief
     * Apply render parameters used by this program to OpenGL.
     *
     * This method is invoked shortly before the renderable geometry is
     * submitted for rendering; the GLProgram must apply to the GL state any
     * parameters it uses.
	 *
	 * \param viewer
	 * Location of the viewer in object space.
	 *
	 * \param localToWorld
	 * Local to world transformation matrix.
     *
     * \param lightParms
     * Params structure containing lighting information.
	 *
	 */
	virtual void applyRenderParams(const Vector3& viewer,
  							       const Matrix4& localToWorld,
  							       const Params& lightParms)
    { }
};
