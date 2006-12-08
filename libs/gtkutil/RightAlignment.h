#ifndef RIGHTALIGNMENT_H_
#define RIGHTALIGNMENT_H_

#include <gtk/gtkalignment.h>

namespace gtkutil
{

/** A GtkAlignment wrapper that right-aligns any child widget.
 */

class RightAlignment
{
	// The alignment widget
	GtkWidget* _widget;
	
public:

	/** Constructor. Accepts the child widget to align.
	 */
	RightAlignment(GtkWidget* w)
	: _widget(gtk_alignment_new(1.0, 0.5, 0, 0))
	{
		gtk_container_add(GTK_CONTAINER(_widget), w);
	}
	
	/** Operator cast to GtkWidget*. 
	 */
	operator GtkWidget* () {
		return _widget;
	}
};

}

#endif /*RIGHTALIGNMENT_H_*/
