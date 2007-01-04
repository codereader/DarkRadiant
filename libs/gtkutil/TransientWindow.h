#ifndef TRANSIENTWINDOW_
#define TRANSIENTWINDOW_

#include <string>
#include "gtk/gtkwindow.h"
#include "gtk/gtkwidget.h"

namespace gtkutil
{

/** greebo: Encapsulation of a GtkWindow with title that is transient to the given parent
 */

class TransientWindow
{
	
protected:
	// The text label
	const std::string _title;
	
	// The window that this window is transient for
	GtkWindow* _parent;
	
public:

	// Constructor
	TransientWindow(const std::string& title, GtkWindow* parent) : 
		_title(title),
		_parent(parent)
	{}
	
	// Operator cast to GtkWindow* (use this to create and retrieve the GtkWidget* pointer)
	virtual operator GtkWidget* () {
		GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
		
		gtk_window_set_transient_for(window, _parent);
		
		return GTK_WIDGET(window);
	}
};

} // namespace gtkutil

#endif /*TRANSIENTWINDOW_*/
