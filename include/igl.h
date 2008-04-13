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

#if !defined(INCLUDED_IGL_H)
#define INCLUDED_IGL_H

#include <string>
#include <GL/glew.h>

#include "imodule.h"

const std::string MODULE_OPENGL("OpenGL");

class OpenGLBinding :
	public RegisterableModule
{
public:
	/// \brief OpenGL version, extracted from the GL_VERSION string.
	int major_version, minor_version;

	/// \brief Is true if the global shared OpenGL context is valid.
	bool contextValid;

	OpenGLBinding() : 
		contextValid(false)
	{}

	/// \brief Asserts that there no OpenGL errors have occurred since the last call to glGetError.
	virtual void assertNoErrors() = 0;
	
	// This is called by the gtkutil::GLWidget class, when at least one shared
	// widget carrying the context exists
	virtual void sharedContextCreated() = 0;
	
	// The counterpart of the above call
	virtual void sharedContextDestroyed() = 0;

	GLuint m_font;
	int m_fontHeight;

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

#if defined(_DEBUG)
inline void GlobalOpenGL_debugAssertNoErrors()
{
  GlobalOpenGL().assertNoErrors();
}
#else
inline void GlobalOpenGL_debugAssertNoErrors()
{
}
#endif

#endif
