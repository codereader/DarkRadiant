#ifndef FRAMEDTRANSIENTWIDGET_H_
#define FRAMEDTRANSIENTWIDGET_H_

#include <string>
#include "gtk/gtkwindow.h"
#include "gtk/gtkframe.h"
#include "gtk/gtkwidget.h"
#include "TransientWindow.h"

namespace gtkutil
{

/** greebo: Encapsulation of a framed, transient Window with a frame around the contained widget
 * 
 * Pass the widget to the class constructor and use the operator GtkWidget* to retrieve 
 * the completed Window widget. 
 */

class FramedTransientWidget
{
	
protected:
	// The text label
	const std::string _title;
	
	// The window that this window is transient for
	GtkWindow* _parent;
	
	// The contained widget
	GtkWidget* _containedWidget;
	
public:

	// Constructor
	FramedTransientWidget(const std::string& title, GtkWindow* parent, GtkWidget* containedWidget) : 
		_title(title),
		_parent(parent),
		_containedWidget(containedWidget)
	{}
	
	// Operator cast to GtkWindow* (use this to create and retrieve the GtkWidget* pointer)
	virtual operator GtkWidget* () {
		// Create a new top level window that is transient for _parent
		GtkWidget* window = TransientWindow(_title, _parent);
		
		// Create a new frame and set its properties
		GtkFrame* frame = GTK_FRAME(gtk_frame_new(0));
		gtk_widget_show(GTK_WIDGET(frame));
		gtk_frame_set_shadow_type(frame, GTK_SHADOW_IN);
		
		// Add the contained widget as children to the frame
		gtk_container_add(GTK_CONTAINER(frame), _containedWidget);
		gtk_widget_show(_containedWidget);
	
		// Now pack the frame into the transient window
		gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(frame));
		
		// Now show the whole widget tree
		gtk_widget_show_all(window);
		
		// Return the readily fabricated widget
		return window;
	}
};

} // namespace gtkutil

#endif /*FRAMEDTRANSIENTWIDGET_H_*/
