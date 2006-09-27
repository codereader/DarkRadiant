#ifndef GLWIDGET_H_
#define GLWIDGET_H_

#include <gtk/gtk.h>
#include <gtk/gtkgl.h>

namespace gtkutil
{

/** Class encapsulating a GTKGlExt widget, providing necessary construction and destruction
 * operations to set up the GL context appropriately.
 */

class GLWidget
{
	// The GTK widget
	GtkWidget* _widget;
	
	// Static, shared GL context for application-wide resource sharing (textures etc)
	static GdkGLContext* _sharedContext;
	
public:

	/** Construct a new GLWidget, creating the GL context as necessary.
	 * 
	 * @param zBuf
	 * True if a Z buffer is required, False otherwise.
	 */
	GLWidget(bool zBuf);
	
	/** Operator cast to GtkWidget* for use with GTK functions.
	 */
	operator GtkWidget* () {
		return _widget;
	}
	
	/** Return the raw widget for certain GTK functions.
	 */
	GtkWidget* getWidget() {
		return _widget;
	}
};

}

#endif /*GLWIDGET_H_*/
