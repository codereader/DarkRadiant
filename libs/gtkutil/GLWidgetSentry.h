#ifndef GLWIDGETSENTRY_H_
#define GLWIDGETSENTRY_H_

#include "GLWidget.h"

namespace gtkutil
{

/** Sentry class that calls glwidget_make_current() on construction and
 * glwidget_swap_buffers() on destruction at the end of a scope. This avoids
 * the need to manually call these functions and use branches to make sure
 * they are executed.
 */

class GLWidgetSentry
{
	// The GL widget
	GtkWidget* _widget;
	
	// Whether the context could be successfully switched
	bool _success;
	
public:

	/** Constructor calls glwidget_make_current().
	 */
	GLWidgetSentry(GtkWidget* widget) : 
		_widget(widget)
	{
		_success = GLWidget::makeCurrent(_widget);
	}
	
	// Returns TRUE if the context could not be switched
	bool failed() const {
		return !_success;
	}
		
	/* Destructor swaps the buffers with glwidget_swap_buffers().
	 */
	~GLWidgetSentry() {
		 GLWidget::swapBuffers(_widget);
	}
};

}

#endif /*GLWIDGETSENTRY_H_*/
