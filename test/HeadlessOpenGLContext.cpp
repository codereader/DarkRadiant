#include "HeadlessOpenGLContext.h"

#ifdef WIN32
#include <GL/wglew.h>
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

#endif

void HeadlessOpenGLContextModule::initialiseModule(const IApplicationContext& ctx)
{}

void HeadlessOpenGLContextModule::createContext()
{
#ifdef WIN32
	GlobalOpenGLContext().setSharedContext(std::make_shared<HeadlessOpenGLContext>());
#else
#error "Headless openGL context not implemented for this platform."
#endif
}

}
