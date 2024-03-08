#include "GLWidget.h"

#include "igl.h"
#include "itextstream.h"
#include "ui/iwxgl.h"

#include "GLContext.h"

#include <wx/dcclient.h>

namespace wxutil
{

const int ATTRIBS [] = {
	WX_GL_RGBA,
	WX_GL_DOUBLEBUFFER,
	WX_GL_DEPTH_SIZE, 24,
	0
};

GLWidget::GLWidget(wxWindow *parent, const std::function<bool()>& renderCallback, const std::string& name) :
    wxGLCanvas(parent, wxID_ANY, ATTRIBS, wxDefaultPosition, wxDefaultSize,
               wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS,
               wxString(name.c_str(), *wxConvCurrent)),
    _registered(false),
    _renderCallback(renderCallback),
    _privateContext(nullptr)
{
    Bind(wxEVT_PAINT, &GLWidget::OnPaint, this);
}

void GLWidget::SetHasPrivateContext(bool hasPrivateContext)
{
	if (hasPrivateContext)
	{
		_privateContext = new wxGLContext(this);
	}
	else
	{
		DestroyPrivateContext();
	}
}

void GLWidget::DestroyPrivateContext()
{
	if (_privateContext != nullptr)
	{
		_privateContext->UnRef();
		_privateContext = nullptr;
	}
}

GLWidget::~GLWidget()
{
	DestroyPrivateContext();

	if (_registered)
	{
		GlobalWxGlWidgetManager().unregisterGLWidget(this);
	}
}

void GLWidget::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	// Got this check from the wxWidgets sources, they assert the widget to be shown
	// "although on MSW it works even if the window is still hidden, it doesn't
    // work in other ports (notably X11-based ones) and documentation mentions
    // that SetCurrent() can only be called for a shown window, so check for it"
	if (!IsShownOnScreen()) return;

	// Make sure this widget is registered
	if (!_registered)
	{
		_registered = true;

		GlobalWxGlWidgetManager().registerGLWidget(this);
	}

    // This is required even though dc is not used otherwise.
    wxPaintDC dc(this);

	// Grab the context for this widget
	if (_privateContext != nullptr)
	{
		// Use the private context for this widget
		SetCurrent(*_privateContext);
	}
	else
	{
		// Use the globally shared context, we rely on this being of type GLContext
		const auto& context = GlobalOpenGLContext().getSharedContext();
		assert(std::dynamic_pointer_cast<GLContext>(context));

		auto wxContext = std::static_pointer_cast<GLContext>(context);
		SetCurrent(wxContext->get());
	}

	if (_renderCallback())
	{
		// Render callback returned true, so drawing took place
		// and we can swap the buffers
		SwapBuffers();
	}
}

} // namespace
