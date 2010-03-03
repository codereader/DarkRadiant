#include "GuiView.h"

#include "igl.h"
#include <gtk/gtkhbox.h>
#include "gtkutil/GLWidgetSentry.h"

namespace gui
{

GuiView::GuiView() :
	_glWidget(new gtkutil::GLWidget(true, "GUI"))
{
	// Construct the widgets
	_widget = gtk_hbox_new(FALSE, 6);
	gtk_widget_set_size_request(*_glWidget, 640, 480);

	gtk_box_pack_start(GTK_BOX(_widget), *_glWidget, TRUE, TRUE, 0);

	GtkWidget* glWidget = *_glWidget;
	gtk_widget_set_events(glWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | 
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | 
		GDK_POINTER_MOTION_MASK);
	g_signal_connect(G_OBJECT(glWidget), "expose-event", G_CALLBACK(onGLDraw), this);

	// Ignore visibility flag and turn invisible background images to visible ones
	_renderer.setIgnoreVisibility(true);
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

void GuiView::initTime(const std::size_t time)
{
	if (_gui != NULL)
	{
		_gui->initTime(time);
	}
}

void GuiView::update(const std::size_t timestep)
{
	if (_gui != NULL)
	{
		_gui->update(timestep);
	}
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

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GuiView::onGLDraw(GtkWidget*, GdkEventExpose*, GuiView* self)
{
	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(*self->_glWidget);

	self->_renderer.render();
}

}
