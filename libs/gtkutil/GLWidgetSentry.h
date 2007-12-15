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
	
public:

	/** Constructor calls glwidget_make_current().
	 */
	GLWidgetSentry(GtkWidget* w)
	: _widget(w)
	{
		glwidget_make_current(_widget);
	}
		
	/* Destructor swaps the buffers with glwidget_swap_buffers().
	 */
	~GLWidgetSentry() {
		glwidget_swap_buffers(_widget);
	}
};

}

#endif /*GLWIDGETSENTRY_H_*/
