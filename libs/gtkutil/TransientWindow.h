#ifndef TRANSIENTWINDOW_
#define TRANSIENTWINDOW_

#include <string>
#include "gtk/gtkwindow.h"
#include "gtk/gtkwidget.h"

#include "gtkutil/pointer.h" // for gpointer_to_int

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
	
	// The actual transient window
	GtkWidget* _window;
	
public:

	// Constructor
	TransientWindow(const std::string& title, GtkWindow* parent) : 
		_title(title),
		_parent(parent),
		_window(gtk_window_new(GTK_WINDOW_TOPLEVEL))
	{}
	
	// Operator cast to GtkWindow* (use this to create and retrieve the GtkWidget* pointer)
	virtual operator GtkWidget* () {
		gtk_window_set_transient_for(GTK_WINDOW(_window), _parent);
		
		// Connect the "resize"-event of the _parent window to the callback, so that the 
		// child can be hidden as well 
		gulong resizeHandler = g_signal_connect(G_OBJECT(_parent), "window_state_event", G_CALLBACK(onParentResize), _window);
		
		// Pack the handler ID into the window, so that it can be disconnected upon destroy 
		g_object_set_data(G_OBJECT(_window), "resizeHandler", gint_to_pointer(resizeHandler));
		
		// Connect the "destroy"-event, so that the handler can be disconnected properly, be sure to pass _parent
		g_signal_connect(G_OBJECT(_window), "delete_event", G_CALLBACK(onDelete), _parent);
		
		return _window;
	}
	
private:

	/* greebo: This gets called when the _parent window is minimised or otherwise resized. If the
	 * parent is actually minimised, minimise the child as well (and vice versa).
	 * 
	 * Parts of this are taken from original GtkRadiant code (window.cpp)
	 */
	static gboolean onParentResize(GtkWidget* widget, GdkEventWindowState* event, GtkWidget* child) {
		// Check, if the event is of interest
		if ((event->changed_mask & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN)) != 0) {
			// Now let's see what the new state of the main window is
			if ((event->new_window_state & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN)) != 0) {
				// The parent got minimised, minimise the child as well
				minimise(child);
			}
			else {
				// Restore the child as the parent is now visible again
				restore(child);
			}
		}
	
		return false;
	}
	
	/* greebo: The following two functions are copied over from window.cpp (GtkRadiant original code)
	 */

	// Re-show a child window when the main window is restored. It is necessary to
	// call gtk_window_move() after showing the window, since with some Linux
	// window managers, the window position is lost after gtk_window_show().
	static void restore(GtkWidget* window) {
		if (GTK_IS_WIDGET(window)) {
			if (gpointer_to_int(g_object_get_data(G_OBJECT(window), "was_mapped")) != 0) {
				gint x = gpointer_to_int(g_object_get_data(G_OBJECT(window), "old_x_position"));
				gint y = gpointer_to_int(g_object_get_data(G_OBJECT(window), "old_y_position"));
				gtk_widget_show(window);
				gtk_window_move(GTK_WINDOW(window), x, y);
			}
		}
	}
	
	// Minimise a child window, storing its position as GObject associated data in
	// order to allow it to be restored correctly.
	static void minimise(GtkWidget* window) {
		if (GTK_IS_WINDOW(window)) {
			if (GTK_WIDGET_VISIBLE(window)) {
				g_object_set_data(G_OBJECT(window), "was_mapped", gint_to_pointer(1));
				gint x, y;
				gtk_window_get_position(GTK_WINDOW(window), &x, &y);
				g_object_set_data(G_OBJECT(window), "old_x_position", gint_to_pointer(x));
				g_object_set_data(G_OBJECT(window), "old_y_position", gint_to_pointer(y));
				gtk_widget_hide(window);
			}
		}
	}

	/* greebo: This disconnects the onResize handler from the _parent window
	 */	
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, GtkWindow* parent) {
		// Retrieve the signal handler id from the child widget
		gulong handlerID = gpointer_to_int(g_object_get_data(G_OBJECT(widget), "resizeHandler"));
		
		// Disconnect the parent
		g_signal_handler_disconnect(G_OBJECT(parent), handlerID);
		
		return false;
	}

};

} // namespace gtkutil

#endif /*TRANSIENTWINDOW_*/
