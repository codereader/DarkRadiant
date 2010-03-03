#include "GuiView.h"

#include "igl.h"
#include "math/matrix.h"
#include <gtk/gtkhbox.h>
#include "gtkutil/GLWidgetSentry.h"

namespace gui
{

	namespace
	{
		const gint DEFAULT_WIDTH = 640;
		const gint DEFAULT_HEIGHT = 480;
	}

GuiView::GuiView() :
	_glWidget(new gtkutil::GLWidget(true, "GUI"))
{
	// Construct the widgets
	_widget = gtk_hbox_new(FALSE, 6);
	gtk_widget_set_size_request(*_glWidget, DEFAULT_WIDTH, DEFAULT_HEIGHT);

	gtk_box_pack_start(GTK_BOX(_widget), *_glWidget, TRUE, TRUE, 0);

	GtkWidget* glWidget = *_glWidget;
	gtk_widget_set_events(glWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | 
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | 
		GDK_POINTER_MOTION_MASK);
	g_signal_connect(G_OBJECT(glWidget), "expose-event", G_CALLBACK(onGLDraw), this);

	// greebo: The "size-allocate" event is needed to determine the window size, as expose-event is 
	// often called for subsets of the widget and the size info in there is therefore not reliable.
	g_signal_connect(G_OBJECT(glWidget), "size-allocate", G_CALLBACK(onSizeAllocate), this);

	// Ignore visibility flag and turn invisible background images to visible ones
	_renderer.setIgnoreVisibility(true);
}

void GuiView::redraw()
{
	gtk_widget_queue_draw(*_glWidget);
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

void GuiView::onSizeAllocate(GtkWidget* widget, GtkAllocation* allocation, GuiView* self)
{
	// Store the window dimensions for later calculations
	self->_windowDims = Vector2(allocation->width, allocation->height);

	// Queue an expose event
	gtk_widget_queue_draw(widget);
}

void GuiView::onGLDraw(GtkWidget*, GdkEventExpose*, GuiView* self)
{
	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(*self->_glWidget);

	double width = self->_windowDims[0];
	double height = self->_windowDims[1];

	if (width / height > static_cast<double>(DEFAULT_WIDTH) / DEFAULT_HEIGHT)
	{
		width = static_cast<double>(DEFAULT_WIDTH) / DEFAULT_HEIGHT * height;
	}
	else
	{
		height = static_cast<double>(DEFAULT_HEIGHT) / DEFAULT_WIDTH * width;
	}

	glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

	// Set up the scale
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	self->_renderer.render();
}

} // namespace
