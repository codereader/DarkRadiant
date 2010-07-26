#ifndef FRAMEDWIDGET_H_
#define FRAMEDWIDGET_H_

#include "gtk/gtkframe.h"
#include "gtk/gtkwidget.h"

#include <gtkmm/frame.h>

namespace gtkutil
{

/** greebo: Encapsulation to create a framed, contained widget
 * 
 * Pass the widget to the class constructor and use the operator GtkWidget* to retrieve 
 * the completed framed widget.
 */

class FramedWidget
{
	
protected:
	// The contained widget
	GtkWidget* _containedWidget;
	
public:

	// Constructor
	FramedWidget(GtkWidget* containedWidget) : 
		_containedWidget(containedWidget)
	{}

	// Destructor
	virtual ~FramedWidget() {}
	
	// Operator cast to GtkWindow* (use this to create and retrieve the GtkWidget* pointer)
	virtual operator GtkWidget* () {
		// Create a new frame and set its properties
		GtkFrame* frame = GTK_FRAME(gtk_frame_new(0));
		gtk_widget_show(GTK_WIDGET(frame));
		gtk_frame_set_shadow_type(frame, GTK_SHADOW_IN);
		
		// Add the contained widget as children to the frame
		gtk_container_add(GTK_CONTAINER(frame), _containedWidget);
		gtk_widget_show(_containedWidget);
	
		// Now show the whole widget tree
		gtk_widget_show_all(GTK_WIDGET(frame));
		
		// Return the readily fabricated widget
		return GTK_WIDGET(frame);
	}
};

// gtkmm variant
class FramedWidgetmm :
	public Gtk::Frame
{
protected:
	// The contained widget
	Gtk::Widget& _containedWidget;
	
public:
	// Constructor
	FramedWidgetmm(Gtk::Widget& containedWidget) : 
		_containedWidget(containedWidget)
	{
		show();
		set_shadow_type(Gtk::SHADOW_IN);
		
		// Add the contained widget as children to the frame
		add(_containedWidget);
		_containedWidget.show();
	
		// Now show the whole widget tree
		show_all();
	}
};

} // namespace gtkutil

#endif /*FRAMEDWIDGET_H_*/
