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
public:
	HeadlessOpenGLContext()
	{
		MSG msg = { 0 };
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = WndProc;
		wc.hInstance = ::GetModuleHandle(nullptr);
		wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
		wc.lpszClassName = L"testwindow";
		wc.style = CS_OWNDC;

		if (!RegisterClass(&wc)) throw std::runtime_error("Failed to register the window class");

		CreateWindowW(wc.lpszClassName, L"testwindow", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, wc.hInstance, 0);

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
