#pragma once

#include <GL/glew.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include <wx/glcanvas.h>
#include <boost/function.hpp>

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
	boost::function<void()> _renderCallback;

public:
    GLWidget(wxWindow *parent, const boost::function<void()>& renderCallback, const std::string& name);

	virtual ~GLWidget();

private:
	void OnPaint(wxPaintEvent& event);
};
typedef boost::shared_ptr<GLWidget> GLWidgetPtr;

} // namespace

