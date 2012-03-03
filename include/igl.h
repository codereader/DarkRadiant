#pragma once

/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string>
#include <GL/glew.h>

#include "imodule.h"

const std::string MODULE_OPENGL("OpenGL");

namespace gtkutil { class GLWidget; }

class OpenGLBinding :
    public RegisterableModule
{
public:
    virtual ~OpenGLBinding() {}

    /// \brief Asserts that there no OpenGL errors have occurred since the last call to glGetError.
    virtual void assertNoErrors() = 0;

    /// Returns the shared context widget holding the GL context
    virtual gtkutil::GLWidget* getGLContextWidget() = 0;

    /// Registers a GL widget, storing the shared context if necessary
    virtual void registerGLWidget(gtkutil::GLWidget* widget) = 0;

    /// Notifies the GL module that a GLWidget has been destroyed
    virtual void unregisterGLWidget(gtkutil::GLWidget* widget) = 0;

    /// \brief Is true if the global shared OpenGL context is valid.
    virtual bool contextValid() const = 0;

    virtual int getFontHeight() = 0;

    /// \brief Renders \p string at the current raster-position of the current context.
    virtual void drawString(const std::string& string) const = 0;

    /// \brief Renders \p character at the current raster-position of the current context.
    virtual void drawChar(char character) const = 0;
};

inline OpenGLBinding& GlobalOpenGL() {
    // Cache the reference locally
    static OpenGLBinding& _openGL(
        *boost::static_pointer_cast<OpenGLBinding>(
            module::GlobalModuleRegistry().getModule(MODULE_OPENGL)
        )
    );
    return _openGL;
}

