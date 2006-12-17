#ifndef SCROLLEDFRAME_H_
#define SCROLLEDFRAME_H_

#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkframe.h>

namespace gtkutil
{

/** Container widget which packs its child into a GtkScrolledWindow and then
 * a GtkFrame. Mainly useful for Tree Views.
 */

class ScrolledFrame
{
	// Main widget
	GtkWidget* _widget;
	
public:

	/** Construct a ScrolledFrame around the provided child widget.
	 */
	ScrolledFrame(GtkWidget* child)
	{
		// Create the GtkScrolledWindow
		GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
									   GTK_POLICY_AUTOMATIC,
									   GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scroll), child);
		
		// Create the GtkFrame
		_widget	= gtk_frame_new(NULL);
		gtk_container_add(GTK_CONTAINER(_widget), scroll);
	}
	
	/** Operator cast to GtkWidget*.
	 */
	operator GtkWidget* () {
		return _widget;	
	}
};

}

#endif /*SCROLLEDFRAME_H_*/
