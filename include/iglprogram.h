#pragma once

class Matrix4;
template<typename Element> class BasicVector3;
typedef BasicVector3<float> Vector3;
template<typename Element> class BasicVector4;
typedef BasicVector4<float> Vector4;

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

	/**
	 * \brief
     * Apply render parameters used by this program to OpenGL.
     *
     * This method is invoked shortly before the renderable geometry is
     * submitted for rendering; the GLProgram must apply to the GL state any
     * parameters it uses.
	 *
	 * @param viewer
	 * Location of the viewer in object space.
	 *
	 * @param localToWorld
	 * Local to world transformation matrix.
	 *
	 * @param origin
	 * Origin of the light in 3D space.
	 *
	 * @param colour
	 * Colour of the light material.
	 *
	 * @param world2light
	 * Transformation from world space into light space, based on the position
	 * and transformations of the light volume.
	 *
	 * @param ambientFactor
	 * 0.0 for a normal light, 1.0 for an ambient light. This affects whether
	 * the lighting is directional or not.
	 */
	virtual void applyRenderParams(const Vector3& viewer,
  							       const Matrix4& localToWorld,
  							       const Vector3& origin,
  							       const Vector4& colour,
  							       const Matrix4& world2light,
  							       float ambientFactor) = 0;
};
