#include "TransientWindow.h"

#include "gtkutil/pointer.h" // for gpointer_to_int
#include <iostream>

namespace gtkutil {

TransientWindow::TransientWindow(const std::string& title, GtkWindow* parent, bool deletable) :
    _title(title),
    _parent(parent),
    _window(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
    _deletable(deletable) 
{
	gtk_window_set_transient_for(GTK_WINDOW(_window), _parent);
	gtk_window_set_title(GTK_WINDOW(_window), _title.c_str());

#ifdef POSIX
    
    // This stops the child windows from appearing in the task bar. Not used on
    // Windows because it possibly causes the windows not to have maximise
    // buttons, which is a user-requested feature
    gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_UTILITY);

#endif
    
}

// Operator cast to GtkWindow* (use this to create and retrieve the GtkWidget* pointer)
TransientWindow::operator GtkWidget* () {
	// Connect the "resize"-event of the _parent window to the callback, so that the
	// child can be hidden as well
	gulong resizeHandler = g_signal_connect(G_OBJECT(_parent), "window-state-event", G_CALLBACK(onParentResize), _window);

	// Pack the handler ID into the window, so that it can be disconnected upon destroy
	g_object_set_data(G_OBJECT(_window), "resizeHandler", gint_to_pointer(resizeHandler));

	if (_deletable) {
		// Connect the "destroy"-event, so that the handler can be disconnected properly, be sure to pass _parent
		g_signal_connect(G_OBJECT(_window), "delete-event", G_CALLBACK(onDelete), _parent);
	}
	
	g_signal_connect(G_OBJECT(_window), "delete-event", G_CALLBACK(showParentOnDelete), _parent);

	return _window;
}

gboolean TransientWindow::onParentResize(GtkWidget* widget, GdkEventWindowState* event, GtkWidget* child) {
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

void TransientWindow::restore(GtkWidget* window) {
	if (GTK_IS_WIDGET(window)) {
		if (gpointer_to_int(g_object_get_data(G_OBJECT(window), "was_mapped")) != 0) {
			gint x = gpointer_to_int(g_object_get_data(G_OBJECT(window), "old_x_position"));
			gint y = gpointer_to_int(g_object_get_data(G_OBJECT(window), "old_y_position"));

			// Be sure to un-flag the window as "mapped", otherwise it will be restored again
			g_object_set_data(G_OBJECT(window), "was_mapped", gint_to_pointer(0));

			gtk_window_move(GTK_WINDOW(window), x, y);

			// Set the window to visible, as it was visible before minimising the parent
			gtk_widget_show(window);
			// Workaround for some linux window managers resetting window positions after show()
			gtk_window_move(GTK_WINDOW(window), x, y);
		}
	}
}

void TransientWindow::minimise(GtkWidget* window) {
	if (GTK_IS_WINDOW(window)) {
		if (GTK_WIDGET_VISIBLE(window)) {
			// Set the "mapped" flag to mark this window as "to be restored again"
			g_object_set_data(G_OBJECT(window), "was_mapped", gint_to_pointer(1));

			// Store the position into the child window
			gint x, y;
			gtk_window_get_position(GTK_WINDOW(window), &x, &y);
			g_object_set_data(G_OBJECT(window), "old_x_position", gint_to_pointer(x));
			g_object_set_data(G_OBJECT(window), "old_y_position", gint_to_pointer(y));

			gtk_widget_hide(window);
		}
	}
}

gboolean TransientWindow::showParentOnDelete(GtkWidget* widget, GdkEvent* event, GtkWindow* parent) {
	// Show the parent again, so that it doesn't disappear behind other applications
	if (gtk_window_is_active(GTK_WINDOW(widget))) {
		gtk_window_present(parent);
	}

	return FALSE;
}

gboolean TransientWindow::onDelete(GtkWidget* widget, GdkEvent* event, GtkWindow* parent) {
	// Retrieve the signal handler id from the child widget
	gulong handlerID = gpointer_to_int(g_object_get_data(G_OBJECT(widget), "resizeHandler"));

	// Disconnect the parent
	g_signal_handler_disconnect(G_OBJECT(parent), handlerID);

	return FALSE;
}

gboolean TransientWindow::toggleOnDelete(GtkWidget* widget, GdkEvent* event, GtkWindow* parent) {
	// Toggle the visibility
	if (GTK_WIDGET_VISIBLE(widget)) {
		gtk_widget_hide(widget);
		
		// Show the parent again, so that it doesn't disappear behind other applications
		if (gtk_window_is_active(GTK_WINDOW(widget))) {
			gtk_window_present(parent);
		}
	}
	else {
		gtk_widget_show(widget);
	}
	
	// Don't propagate the call
	return TRUE;
}

} // namespace gtkutil
