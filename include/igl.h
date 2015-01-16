#pragma once

#include <string>
#include <GL/glew.h>

#include "imodule.h"

const std::string MODULE_OPENGL("OpenGL");

namespace wxutil { class GLWidget; }
class wxGLContext;

class OpenGLBinding :
    public RegisterableModule
{
public:
    virtual ~OpenGLBinding() {}

    /// \brief Asserts that there no OpenGL errors have occurred since the last call to glGetError.
    virtual void assertNoErrors() = 0;

	/// Returns the shared context widget holding the GL context
    virtual wxGLContext& getwxGLContext() = 0;

    /// Registers a GL widget, storing the shared context if necessary
    virtual void registerGLCanvas(wxutil::GLWidget* widget) = 0;

    /// Notifies the GL module that a GLWidget has been destroyed
    virtual void unregisterGLCanvas(wxutil::GLWidget* widget) = 0;

	/// \brief Is true if the global shared OpenGL context is valid.
    virtual bool wxContextValid() const = 0;

    // Returns true if openGL supports ARB or GLSL lighting
    virtual bool shaderProgramsAvailable() const = 0;

    // Sets the flag whether shader programs are available. 
    // This is set by the RenderSystem once the extensions are initialised
    virtual void setShaderProgramsAvailable(bool available) = 0;

    virtual int getFontHeight() = 0;

    /// \brief Renders \p string at the current raster-position of the current context.
    virtual void drawString(const std::string& string) const = 0;

    /// \brief Renders \p character at the current raster-position of the current context.
    virtual void drawChar(char character) const = 0;
};

inline OpenGLBinding& GlobalOpenGL() {
    // Cache the reference locally
    static OpenGLBinding& _openGL(
        *std::static_pointer_cast<OpenGLBinding>(
            module::GlobalModuleRegistry().getModule(MODULE_OPENGL)
        )
    );
    return _openGL;
}

