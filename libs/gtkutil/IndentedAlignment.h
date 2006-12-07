#ifndef INDENTEDALIGNMENT_H_
#define INDENTEDALIGNMENT_H_

#include <gtk/gtkalignment.h>

namespace gtkutil
{

/** A GtkAlignment that indents its child by a specified amount (in pixels).
 */

class IndentedAlignment
{
	// The GtkAlignment object
	GtkWidget* _widget;
	
public:

	/** Constructor. Specifies the child widget and the amount to indent in
	 * pixels.
	 */
	IndentedAlignment(GtkWidget* w, int pixels)
	: _widget(gtk_alignment_new(0.0, 0.5, 1.0, 1.0))
	{
		// Set the padding
		gtk_alignment_set_padding(GTK_ALIGNMENT(_widget),
								  0, 0, pixels, 0); // t, b, l, r
		// Add the child
		gtk_container_add(GTK_CONTAINER(_widget), w);
	}
	
	/** Operator cast to GtkWidget*. Returns the constructed GtkAlignment
	 * with child.
	 */
	operator GtkWidget* () {
		return _widget;
	}
};

}

#endif /*INDENTEDALIGNMENT_H_*/
