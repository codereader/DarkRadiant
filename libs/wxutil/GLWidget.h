#pragma once

#include <GL/glew.h>
#include <memory>
#include <string>
#include <wx/glcanvas.h>
#include <functional>

// greebo: Undo the min max macro definitions coming from a windows header
#undef min
#undef max

namespace wxutil
{

class GLWidget :
	public wxGLCanvas
{
	// TRUE, if this GL widget has been registered
	bool _registered;

	// The attached client method to invoke to render this view
	std::function<bool()> _renderCallback;

	// Some widgets have their own openGL context, 
	// If it  is non-NULL _privateContext will be used. 
	wxGLContext* _privateContext;

public:
    GLWidget(wxWindow *parent, const std::function<bool()>& renderCallback, const std::string& name);

	// Call this to enable/disable the private GL context of this widget
	void SetHasPrivateContext(bool hasPrivateContext);

	virtual ~GLWidget();

private:
	void DestroyPrivateContext();
	void OnPaint(wxPaintEvent& event);
};
typedef std::shared_ptr<GLWidget> GLWidgetPtr;

} // namespace

