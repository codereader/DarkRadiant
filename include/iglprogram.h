#pragma once

class Matrix4;

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
	 * Create this program using glGenPrograms and siblings. This needs the
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
};

// Interface implemented by GLSL programs supporting alpha test
class ISupportsAlphaTest
{
public:
    virtual ~ISupportsAlphaTest() {}

    // Set the alpha test value uniform
    virtual void setAlphaTest(float alphaTest) = 0;

    // Set the diffuse texture matrix needed to look up the texel for the alpha test
    virtual void setDiffuseTextureTransform(const Matrix4& transform) = 0;
};
