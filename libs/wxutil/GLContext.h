#pragma once

#include "igl.h"
#include "itextstream.h"
#include <wx/glcanvas.h>

namespace wxutil
{

// IGLContext implementation based on wxGLContext
class GLContext :
	public gl::IGLContext
{
private:
	wxGLContext* _context;

public:
	GLContext(wxutil::GLWidget* hostWidget) :
		_context(new wxGLContext(hostWidget))
	{
		hostWidget->SetCurrent(*_context);

		// report OpenGL information
		rMessage() << "GL_VENDOR: "
			<< reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << std::endl;
		rMessage() << "GL_RENDERER: "
			<< reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << std::endl;
		rMessage() << "GL_VERSION: "
			<< reinterpret_cast<const char*>(glGetString(GL_VERSION)) << std::endl;
#if NDEBUG
		rMessage() << "GL_EXTENSIONS: "
			<< reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) << std::endl;
#endif
		GLenum err = glewInit();

		if (err != GLEW_OK)
		{
			// glewInit failed
			rError() << "GLEW error: " <<
				reinterpret_cast<const char*>(glewGetErrorString(err));
		}
	}

	~GLContext()
	{
		delete _context;
	}

	wxGLContext& get()
	{
		return *_context;
	}
};

}
