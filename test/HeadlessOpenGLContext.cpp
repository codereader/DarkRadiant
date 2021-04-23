#include "HeadlessOpenGLContext.h"
#include "itextstream.h"

#ifdef WIN32
#include <GL/wglew.h>
#elif defined(POSIX)
#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glew.h>
#include <GL/glxew.h>
#endif

namespace gl
{

#ifdef WIN32
class HeadlessOpenGLContext :
	public IGLContext
{
private:
	HGLRC _context;
	static HGLRC _tempContext;

	WNDCLASS _wc;
public:
	HeadlessOpenGLContext()
	{
		MSG msg = { 0 };
		_wc = { 0 };
		_wc.lpfnWndProc = WndProc;
		_wc.hInstance = ::GetModuleHandle(nullptr);
		_wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
		_wc.lpszClassName = L"HeadlessOpenGLContext";
		_wc.style = CS_OWNDC;

		if (!GetClassInfo(_wc.hInstance, _wc.lpszClassName, &_wc))
		{
			if (!RegisterClass(&_wc)) throw std::runtime_error("Failed to register the window class");
		}

		CreateWindowW(_wc.lpszClassName, L"HeadlessOpenGLContext", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, _wc.hInstance, 0);

		while (_tempContext == nullptr && PeekMessage(&msg, NULL, 0, 0, 0) > 0)
		{
			DispatchMessage(&msg);
		}

		_context = _tempContext;
		_tempContext = nullptr;
	}

	~HeadlessOpenGLContext()
	{
		if (_context)
		{
			wglDeleteContext(_context);
		}
	}

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CREATE:
		{
			PIXELFORMATDESCRIPTOR pfd =
			{
				sizeof(PIXELFORMATDESCRIPTOR),
				1,
				PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
				PFD_TYPE_RGBA,
				32,                   // color depth.
				0, 0, 0, 0, 0, 0,
				0,
				0,
				0,
				0, 0, 0, 0,
				24,                   // depth buffer bits
				8,                    // stencil bits
				0,                    // aux buffers.
				PFD_MAIN_PLANE,
				0,
				0, 0, 0
			};

			auto deviceContext = GetDC(hWnd);
			int pixelFormat = ChoosePixelFormat(deviceContext, &pfd);

			SetPixelFormat(deviceContext, pixelFormat, &pfd);

			_tempContext = wglCreateContext(deviceContext);
			wglMakeCurrent(deviceContext, _tempContext);
		}
		break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
};

HGLRC HeadlessOpenGLContext::_tempContext;

#elif defined(POSIX)

class HeadlessOpenGLContext :
	public IGLContext
{
private:
	GLXContext _context;
	Display* _display;
	GLXFBConfig* _fbConfigs;
	GLXPbuffer _pixelBuffer;

public:
	HeadlessOpenGLContext() :
		_context(nullptr),
		_pixelBuffer(0)
	{
		auto glxcfbconfig = (PFNGLXCHOOSEFBCONFIGPROC)glXGetProcAddress((GLubyte*)"glXChooseFBConfig");
		auto glxcreatenewctx = (PFNGLXCREATENEWCONTEXTPROC)glXGetProcAddress((GLubyte*)"glXCreateNewContext");
		auto glxcreatepixelbuffer = (PFNGLXCREATEPBUFFERPROC)glXGetProcAddress((GLubyte*)"glXCreatePbuffer");
		auto glxmakecurrent = (PFNGLXMAKECONTEXTCURRENTPROC)glXGetProcAddress((GLubyte*)"glXMakeContextCurrent");

		std::cout << "XOpenDisplay..." << std::endl;

        _display = XOpenDisplay(nullptr);

		std::cout << "XOpenDisplay returned: " << _display << std::endl;

		static int pixelFormat[] = { GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT, None };

		int glx_major, glx_minor;
		if (!glXQueryVersion(_display, &glx_major, &glx_minor))
		{
			std::cerr << "glXQueryVersion failed" << std::endl;
			throw new std::runtime_error("Failed to query GLX version");
		}

		std::cout << "GLX version: " << glx_major << "." << glx_minor << std::endl;

		std::cout << "glxcfbconfig..." << std::endl;

		int configs = 0;
        _fbConfigs = glxcfbconfig(_display, DefaultScreen(_display), 0, &configs);

		std::cout << "glxcreatenewctx..." << std::endl;

		_context = glxcreatenewctx(_display, _fbConfigs[0], GLX_RGBA_TYPE, None, True);

		std::cout << "glxcreatepixelbuffer..." << std::endl;

		// Create a dummy pbuffer. We will render to framebuffers anyway, but we need a pbuffer to
        // activate the context.
        int pixelBufferAttributes[] = { GLX_PBUFFER_WIDTH, 8, GLX_PBUFFER_HEIGHT, 8, None };
        _pixelBuffer = glxcreatepixelbuffer(_display, _fbConfigs[0], pixelBufferAttributes);

		std::cout << "glxmakecurrent..." << std::endl;

         // try to make it the current context
        if (!glxmakecurrent(_display, _pixelBuffer, _pixelBuffer, _context))
		{
			std::cout << "1st glxmakecurrent failed..." << std::endl;

			// some drivers does not support context without default framebuffer, so fallback on
			// using the default window.
			if (!glxmakecurrent(_display, DefaultRootWindow(_display), DefaultRootWindow(_display), _context))
			{
				std::cout << "2nd glxmakecurrent failed..." << std::endl;

				rError() << "Failed to make current" << std::endl;
				throw new std::runtime_error("Failed to make current");
			}
        }

		std::cout << "HeadlessOpenGLContext done." << std::endl;
	}

	~HeadlessOpenGLContext()
	{
		if (_pixelBuffer) 
		{
			auto glxdestroypixelbuffer = (PFNGLXDESTROYPBUFFERPROC)glXGetProcAddress((GLubyte*)"glXDestroyPbuffer");
            glxdestroypixelbuffer(_display, _pixelBuffer);
        }

		glXDestroyContext(_display, _context);

		XFree(_fbConfigs);
		XCloseDisplay(_display);
	}
};

#endif

void HeadlessOpenGLContextModule::initialiseModule(const IApplicationContext& ctx)
{}

void HeadlessOpenGLContextModule::createContext()
{
	try
	{
#ifdef WIN32
	GlobalOpenGLContext().setSharedContext(std::make_shared<HeadlessOpenGLContext>());
#elif defined(POSIX)
	GlobalOpenGLContext().setSharedContext(std::make_shared<HeadlessOpenGLContext>());
#else
#error "Headless openGL context not implemented for this platform."
#endif
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << "Headless GL context creation failed: " << ex.what() << std::endl;
	}
}

}
