#ifndef RIGHTALIGNMENT_H_
#define RIGHTALIGNMENT_H_

#include <gtk/gtkalignment.h>

namespace gtkutil
{

/** A GtkAlignment wrapper that right-aligns any child widget.
 */

class RightAlignment
{
	// The child widget
	GtkWidget* _child;
	
public:

	/** Constructor. Accepts the child widget to align.
	 */
	RightAlignment(GtkWidget* w)
	: _child(w)
	{}
	
	/** Operator cast to GtkWidget*. Packs the child into a GtkAlignment
	 * and returns the resulting widget.
	 */
	operator GtkWidget* () {
		GtkWidget* align = gtk_alignment_new(1.0, 0.5, 0, 0);
		gtk_container_add(GTK_CONTAINER(align), _child);
		return align;
	}
};

}

#endif /*RIGHTALIGNMENT_H_*/
