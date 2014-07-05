#include "GLWidget.h"

#include "igl.h"
#include "itextstream.h"

#include <wx/dcclient.h>

namespace wxutil
{

GLWidget::GLWidget(wxWindow *parent, const boost::function<void()>& renderCallback, const std::string& name) : 
	wxGLCanvas(parent, -1, (int*)NULL, wxDefaultPosition, wxDefaultSize,
               wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS, wxString(name.c_str(), *wxConvCurrent)),
	_registered(false),
	_renderCallback(renderCallback)
{
	Connect(wxEVT_PAINT, wxPaintEventHandler(GLWidget::OnPaint), NULL, this);
}

GLWidget::~GLWidget()
{
	if (_registered)
	{
		GlobalOpenGL().unregisterGLCanvas(this);
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

		GlobalOpenGL().registerGLCanvas(this);
	}

    // This is required even though dc is not used otherwise.
    wxPaintDC dc(this);

	// Grab the contex for this widget
	SetCurrent(GlobalOpenGL().getwxGLContext());

	_renderCallback();

    SwapBuffers();
}

} // namespace
