#pragma once

class Matrix4;

/// Representation of a GL vertex/fragment program.
class GLProgram
{
public:
    /// Destructor
    virtual ~GLProgram() {}

    /// Bind this program as the currently-operative shader.
    virtual void enable() = 0;

    /// Unbind this program from OpenGL.
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
