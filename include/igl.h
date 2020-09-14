#pragma once

#include <string>
#include <sigc++/signal.h>
#include <GL/glew.h>

#include "imodule.h"

namespace gl
{

// Base type of any object representing a GL context
class IGLContext
{
public:
    typedef std::shared_ptr<IGLContext> Ptr;

    virtual ~IGLContext() {}
};

// An IGLContextProvider implementation must be able
// to create a valid context and return it as an object
// deriving from IGLContext
// Only one IGLContextProvider-implementing module is 
// allowed in a given set of modules.
class IGLContextProvider :
    public RegisterableModule
{
public:
    virtual ~IGLContextProvider() {}

    // Create a GL context and return it
    virtual IGLContext::Ptr createContext() = 0;
};

// Interface of the module holding the shared GL context
// of this application. When the shared GL context has been
// created or destroyed, the corresponding events are fired.
class ISharedGLContextHolder :
    public RegisterableModule
{
public:
    virtual ~ISharedGLContextHolder() {}

    // Get the shared context object (might be empty)
    virtual const IGLContext::Ptr& getSharedContext() = 0;

    // Set the shared context object. Invoking this method
    // while a shared context is already registered will cause an
    // exception to be thrown
    virtual void setSharedContext(const IGLContext::Ptr& context) = 0;

    // Fired right after the shared context instance has been registered
    virtual sigc::signal<void>& signal_sharedContextCreated() = 0;

    // Fired when the shared context instance has been destroyed
    virtual sigc::signal<void>& signal_sharedContextDestroyed() = 0;
};

}

const char* const MODULE_GL_CONTEXT_PROVIDER("GLContextProvider");
const char* const MODULE_SHARED_GL_CONTEXT("SharedGLContextHolder");

inline gl::ISharedGLContextHolder& GlobalOpenGLContext()
{
    // Cache the reference locally
    static gl::ISharedGLContextHolder& _instance(
        *std::static_pointer_cast<gl::ISharedGLContextHolder>(
            module::GlobalModuleRegistry().getModule(MODULE_SHARED_GL_CONTEXT)
        )
    );
    return _instance;
}

const char* const MODULE_OPENGL("OpenGL");

namespace wxutil { class GLWidget; }
class wxGLContext;

class OpenGLBinding :
    public RegisterableModule
{
public:
    virtual ~OpenGLBinding() {}

    /// \brief Asserts that there no OpenGL errors have occurred since the last call to glGetError.
	// Normally, you want to use the debug::assertNoGlErrors() wrapper which does nothing in release builds
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

inline OpenGLBinding& GlobalOpenGL() 
{
    // Cache the reference locally
    static OpenGLBinding& _openGL(
        *std::static_pointer_cast<OpenGLBinding>(
            module::GlobalModuleRegistry().getModule(MODULE_OPENGL)
        )
    );
    return _openGL;
}

namespace debug
{

inline void assertNoGlErrors()
{
#ifdef _DEBUG
	GlobalOpenGL().assertNoErrors();
#endif
}

}

