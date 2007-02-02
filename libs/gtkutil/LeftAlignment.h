#ifndef LEFTALIGNMENT_H_
#define LEFTALIGNMENT_H_

#include <gtk/gtkalignment.h>

namespace gtkutil
{
	
/** Container that left-aligns a child widget, with configurable indentation
 * and fill ratio.
 */

class LeftAlignment
{
	// The alignment widget
	GtkWidget* _widget;
	
public:

	/** Constructor. Accepts the child widget to align.
	 * 
	 * @param w
	 * The child widget.
	 * 
	 * @param indent
	 * Number of pixels to left-indent by (default 0).
	 * 
	 * @param expand
	 * Amount by which the child should expand, from 0.0 (no expansion, widget
	 * remains at default size) to 1.0 (widget expands to fill all available
	 * space).
	 */
	LeftAlignment(GtkWidget* w, int indent = 0, float expand = 0.0)
	: _widget(gtk_alignment_new(0.0, 0.5, expand, 1.0))
	{
		gtk_alignment_set_padding(GTK_ALIGNMENT(_widget), 0, 0, indent, 0);
		gtk_container_add(GTK_CONTAINER(_widget), w);
	}
	
	/** Operator cast to GtkWidget*. 
	 */
	operator GtkWidget* () {
		return _widget;
	}
};

}

#endif /*LEFTALIGNMENT_H_*/
