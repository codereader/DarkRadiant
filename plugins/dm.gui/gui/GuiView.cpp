#include "GuiView.h"

#include "igl.h"
#include "math/Matrix4.h"
#include <gtk/gtkhbox.h>
#include "gtkutil/GLWidgetSentry.h"

namespace gui
{

	namespace
	{
		const int DEFAULT_WIDTH = 640;
		const int DEFAULT_HEIGHT = 480;
	}

GuiView::GuiView() :
	Gtk::HBox(false, 6),
	_glWidget(Gtk::manage(new gtkutil::GLWidget(true, "GUI")))
{
	// Construct the widgets
	_glWidget->set_size_request(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	pack_start(*_glWidget, true, true, 0);

	_glWidget->set_events(Gdk::EXPOSURE_MASK | Gdk::BUTTON_PRESS_MASK |
		Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);

	_glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &GuiView::onGLDraw));

	// greebo: The "size-allocate" event is needed to determine the window size, as expose-event is
	// often called for subsets of the widget and the size info in there is therefore not reliable.
	_glWidget->signal_size_allocate().connect(sigc::mem_fun(*this, &GuiView::onSizeAllocate));

	// Ignore visibility flag and turn invisible background images to visible ones
	_renderer.setIgnoreVisibility(true);
}

void GuiView::redraw()
{
	_glWidget->queue_draw();
}

void GuiView::setGui(const GuiPtr& gui)
{
	// Check for equality
	if (gui != _gui)
	{
		_gui = gui;
		_renderer.setGui(gui);
	}
}

const GuiPtr& GuiView::getGui()
{
	return _gui;
}

void GuiView::initialiseView()
{
	// Grab the GL widget with sentry object
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	// Clear the window
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearColor(0.0, 0.0, 0.0, 0);
	glClearDepth(100.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set up the camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
}

void GuiView::setGLViewPort()
{
	double width = _windowDims[0];
	double height = _windowDims[1];
	double aspectRatio = static_cast<double>(DEFAULT_WIDTH) / DEFAULT_HEIGHT;

	if (width / height > aspectRatio)
	{
		width = height * aspectRatio;
	}
	else
	{
		height = width / aspectRatio;
	}

	glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void GuiView::draw()
{
	if (_gui == NULL) return;

	// Prepare the GUI for rendering, like re-compiling texts etc.
	// This has to be performed before states are initialised
	_gui->pepareRendering();

	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	setGLViewPort();

	// Set up the scale
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	_renderer.render();
}

void GuiView::onSizeAllocate(Gtk::Allocation& allocation)
{
	// Store the window dimensions for later calculations
	_windowDims = Vector2(allocation.get_width(), allocation.get_height());

	// Queue an expose event
	_glWidget->queue_draw();
}

bool GuiView::onGLDraw(GdkEventExpose*)
{
	draw();

	return false;
}

} // namespace
