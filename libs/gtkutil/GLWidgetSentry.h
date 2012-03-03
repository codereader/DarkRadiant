#pragma once

#include "GLWidget.h"

namespace gtkutil
{

/**
 * \brief
 * Sentry class that calls GLWidget::makeCurrent() on construction and
 * GLWidget::swapBuffers() on destruction at the end of a scope.
 */
class GLWidgetSentry
{
	// The GL widget
    gtkutil::GLWidget& _widget;

	// Whether the context could be successfully switched
	bool _success;

public:

    /// Construct and make the widget current
	GLWidgetSentry(gtkutil::GLWidget& widget) :
		_widget(widget)
	{
		_success = _widget.makeCurrent();
	}

	// Returns TRUE if the context could not be switched
	bool failed() const
	{
		return !_success;
	}

	/* Destructor swaps the buffers with glwidget_swap_buffers().
	 */
	~GLWidgetSentry()
	{
        _widget.swapBuffers();
	}
};

}
