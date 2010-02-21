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

	setGui("guis/readables/books/book_calig_camberic.gui");
}

void GuiView::setGui(const GuiPtr& gui)
{
	_gui = gui;
	_renderer.setGui(gui);
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
	gluPerspective(60, 1, 0.1, 10000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set up the lights
	/*glEnable(GL_LIGHTING);

	glEnable(GL_LIGHT0);
	GLfloat l0Amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat l0Dif[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat l0Pos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, l0Amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Dif);
	glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);

	glEnable(GL_LIGHT1);
	GLfloat l1Dif[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat l1Pos[] = { 0.0, 0.0, 1.0, 0.0 };
	glLightfv(GL_LIGHT1, GL_DIFFUSE, l1Dif);
	glLightfv(GL_LIGHT1, GL_POSITION, l1Pos);*/
}

void GuiView::onGLDraw(GtkWidget*, GdkEventExpose*, GuiView* self)
{
	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(*self->_glWidget);

	self->_renderer.render();
}

}
