/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

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
#include "qgl.h"

#include <GL/glew.h>

#include "imodule.h"
#include "irender.h"
#include "igl.h"

#include "gtkutil/glfont.h"
#include "debugging/debugging.h"

#include "modulesystem/StaticModule.h"
#include <iostream>

//****************************** Error handling ********************************

typedef struct glu_error_struct
{
    GLenum     errnum;
    const char *errstr;
} GLU_ERROR_STRUCT;

GLU_ERROR_STRUCT glu_errlist[] = {
  {GL_NO_ERROR, "GL_NO_ERROR - no error"},
  {GL_INVALID_ENUM, "GL_INVALID_ENUM - An unacceptable value is specified for an enumerated argument."},
  {GL_INVALID_VALUE, "GL_INVALID_VALUE - A numeric argument is out of range."},
  {GL_INVALID_OPERATION, "GL_INVALID_OPERATION - The specified operation is not allowed in the current state."},
  {GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW - Function would cause a stack overflow."},
  {GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW - Function would cause a stack underflow."},
  {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY - There is not enough memory left to execute the function."},
  {0, 0}
};

const GLubyte* qgluErrorString(GLenum errCode )
{
  int search = 0;
  for (search = 0; glu_errlist[search].errstr; search++)
  {
    if (errCode == glu_errlist[search].errnum)
      return (const GLubyte *)glu_errlist[search].errstr;
  } //end for
  return (const GLubyte *)"Unknown error";
}

//************************************ Loading *************************************

int QGL_Init(OpenGLBinding& table) {
  return 1;
}

void QGL_Shutdown(OpenGLBinding& table) {
  return;
}

//************************************ Module ***************************************

class QglAPI :
	public OpenGLBinding
{
	GLFont g_font;
	
public:
	QglAPI() :
		g_font(0, 0)
	{}
	
	virtual void assertNoErrors() {
#ifdef _DEBUG
		GLenum error = glGetError();
		while (error != GL_NO_ERROR) {
			const char* errorString = reinterpret_cast<const char*>(qgluErrorString(error));
			
			if (error == GL_OUT_OF_MEMORY) {
				ERROR_MESSAGE("OpenGL out of memory error: " << errorString);
			}
			else {
				ERROR_MESSAGE("OpenGL error: " << errorString);
			}
			error = glGetError();
		}
#endif
	}
	
	virtual void sharedContextCreated() {
		// report OpenGL information
		globalOutputStream() << "GL_VENDOR: "
			<< reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << "\n";
		globalOutputStream() << "GL_RENDERER: "
			<< reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << "\n";
		globalOutputStream() << "GL_VERSION: "
			<< reinterpret_cast<const char*>(glGetString(GL_VERSION)) << "\n";
		globalOutputStream() << "GL_EXTENSIONS: "
			<< reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) << "\n";

		GLenum err = glewInit();
		if (err != GLEW_OK)	{
			// glewInit failed
			globalErrorStream() << "GLEW error: " << 
				reinterpret_cast<const char*>(glewGetErrorString(err));
		}

		GlobalShaderCache().extensionsInitialised();
		GlobalShaderCache().realise();

		g_font = glfont_create("courier 8");
		m_font = g_font.getDisplayList();
		m_fontHeight = g_font.getPixelHeight();
	}
		
	virtual void sharedContextDestroyed() {
		GlobalShaderCache().unrealise();
	}
	
	virtual void drawString(const char* string) const {
		glListBase(m_font);
		glCallLists(GLsizei(strlen(string)), GL_UNSIGNED_BYTE, reinterpret_cast<const GLubyte*>(string));
	}
	
	virtual void drawChar(char character) const {
		glListBase(m_font);
		glCallLists(1, GL_UNSIGNED_BYTE, reinterpret_cast<const GLubyte*>(&character));
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_OPENGL);
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "OpenGL::initialiseModule called.\n";
	}
};

// Define the static Radiant module
module::StaticModule<QglAPI> openGLModule;
