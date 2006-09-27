#include "GLWidget.h"

namespace gtkutil
{

// Initialise the static shared context

GdkGLContext* GLWidget::_sharedContext = NULL;

// Construct the GL widget, with or without a Z buffer

GLWidget::GLWidget(bool zBuf)
: _widget(gtk_drawing_area_new())
{
	// Create an appropriate GLConfig
	GdkGLConfig* config;
	if (zBuf) {
		config = gdk_gl_config_new_by_mode(GdkGLConfigMode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE));
	}
	else {
		config = gdk_gl_config_new_by_mode(GdkGLConfigMode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_DEPTH));
	}
	
	// Set the GL capability on the widget
	gtk_widget_set_gl_capability(_widget,
								 config,
								 _sharedContext, // NULL is acceptable if no shared context yet
								 TRUE, // use direct rendering
								 GDK_GL_RGBA_TYPE); // colour mode
								 
	// If the shared context is uninitialised, we use the newly-created context
	_sharedContext = gtk_widget_get_gl_context(_widget);
}

}
