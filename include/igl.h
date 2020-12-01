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
    static module::InstanceReference<gl::ISharedGLContextHolder> _reference(MODULE_SHARED_GL_CONTEXT);
    return _reference;
}

const char* const MODULE_OPENGL("OpenGL");

class IGLFont
{
public:
    using Ptr = std::shared_ptr<IGLFont>;

    enum class Style
    {
        Sans,	// free sans
        Mono,	// free mono
    };

    virtual ~IGLFont() {}

    // Returns the line spacing of this font
    virtual float getLineHeight() const = 0;

    /// \brief Renders \p string at the current raster-position of the current context.
    virtual void drawString(const std::string& string) = 0;
};

class OpenGLBinding :
    public RegisterableModule
{
public:
    virtual ~OpenGLBinding() {}

    // Acquires a shared reference to the font of the given style and size
    virtual IGLFont::Ptr getFont(IGLFont::Style style, std::size_t size) = 0;

    // Deprecated
    virtual int getFontHeight() = 0;

    // Deprecated
    /// \brief Renders \p string at the current raster-position of the current context.
    virtual void drawString(const std::string& string) const = 0;

    // Deprecated
    /// \brief Renders \p character at the current raster-position of the current context.
    virtual void drawChar(char character) const = 0;
};

inline OpenGLBinding& GlobalOpenGL() 
{
    static module::InstanceReference<OpenGLBinding> _reference(MODULE_OPENGL);
    return _reference;
}
