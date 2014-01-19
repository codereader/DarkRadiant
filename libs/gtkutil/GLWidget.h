#pragma once

#include <boost/shared_ptr.hpp>
#include <string>
#include <GL/glew.h>
#include <wx/glcanvas.h>
#include <gtkmm/gl/drawingarea.h>

// greebo: Undo the min max macro definitions coming from a windows header
#undef min
#undef max

namespace gtkutil
{

/// OpenGL drawing widget
class GLWidget :
	public Gtk::GL::DrawingArea
{
	// TRUE, if this GL widget has depth-buffering enabled
	bool _zBuffer;

public:

	// Constructor, pass TRUE to enable depth-buffering
    GLWidget(bool zBuffer, const std::string& debugName = std::string());

    /// Make this GLWidget's context and drawable current
	bool makeCurrent();

    /// Finish drawing and swap buffers
	void swapBuffers();

private:
	// As soon as the widget is packed into a parent, this callback is invoked
	// and enables the GL drawing for this widget
	void onHierarchyChanged(Gtk::Widget* previous_toplevel);

	// Acquires a GDK GL config structure with or without depth
	static Glib::RefPtr<Gdk::GL::Config> createGLConfigWithDepth();
	static Glib::RefPtr<Gdk::GL::Config> createGLConfig();
};
typedef boost::shared_ptr<GLWidget> GLWidgetPtr;

} // namespace gtkutil

namespace wxutil
{

class GLWidget :
	public wxGLCanvas
{
	// TRUE, if this GL widget has depth-buffering enabled
	bool _zBuffer;

public:
    GLWidget(wxWindow *parent, int* attribList);

    /// Make this GLWidget's context and drawable current
	bool makeCurrent();

    /// Finish drawing and swap buffers
	void swapBuffers();

private:
	void OnPaint(wxPaintEvent& event);

	DECLARE_EVENT_TABLE();
};
typedef boost::shared_ptr<GLWidget> GLWidgetPtr;

} // namespace

