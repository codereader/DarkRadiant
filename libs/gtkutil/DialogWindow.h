#ifndef DIALOGWINDOW_H_
#define DIALOGWINDOW_H_

#include "TransientWindow.h"

/* greebo: This the prototype of a dialog window.
 * 
 * Instantiate the window using new DialogWindow() and call destroy()
 * to remove it from the heap.
 */

namespace gtkutil {

class DialogWindow :
	public TransientWindow
{
	
public:
	// Constructors
	DialogWindow(const std::string& title, GtkWindow* parent) :
		TransientWindow(title, parent)
	{
		gtk_window_set_modal(GTK_WINDOW(_window), TRUE);
	    gtk_window_set_position(GTK_WINDOW(_window), GTK_WIN_POS_CENTER_ON_PARENT);
	    
		// Be sure that everything is properly destroyed upon window closure
		g_signal_connect(G_OBJECT(_window), "delete-event", G_CALLBACK(onDelete), this);
	}
	
	virtual void populateWindow() = 0;
	
	virtual void setWindowSize(const unsigned int width, const unsigned int height) {
		gtk_window_set_default_size(GTK_WINDOW(_window), width, height);
		gtk_window_set_position(GTK_WINDOW(_window), GTK_WIN_POS_CENTER_ON_PARENT);
	}

	virtual void destroy() {
		// Hide and destroy the window widget (and all its children)
		if (GTK_IS_WIDGET(_window)) {
			gtk_widget_hide(GTK_WIDGET(_window));
			gtk_widget_destroy(GTK_WIDGET(_window));
		}
		
		delete this;
	}

private:
	/* greebo: Call this to remove the dialog from the heap.
	 */
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, DialogWindow* self) {
		self->destroy();
		
		return false;
	}
	
}; // class DialogWindow 

} // namespace ui

#endif /*DIALOGWINDOW_H_*/
