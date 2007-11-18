#include "PersistentTransientWindow.h"

#include "gtkutil/pointer.h" // for gpointer_to_int
#include <iostream>

namespace gtkutil {

PersistentTransientWindow::PersistentTransientWindow(const std::string& title, 
								 					 GtkWindow* parent,
								 					 bool hideOnDelete) 
: TransientWindow(title, parent, hideOnDelete),
  _parent(parent)
{
	// Connect up a resize handler to the parent window, so that this window
	// will be hidden and restored along with the parent
	_parentResizeHandler = g_signal_connect(
		G_OBJECT(parent), 
		"window-state-event", 
		G_CALLBACK(_onParentResize), 
		this
	);
}

PersistentTransientWindow::~PersistentTransientWindow() {
	// greebo: Call the destroy method of the subclass, before
	// this class gets destructed, otherwise the virtual overridden
	// methods won't get called anymore.
	if (GTK_IS_WIDGET(getWindow())) {
		destroy();
	}
}

// Activate parent if necessary
void PersistentTransientWindow::activateParent() {
	// Only activate if this window is active already
	if (gtk_window_is_active(GTK_WINDOW(getWindow()))) {
		gtk_window_present(_parent);
	}
}

// Post-hide event from TransientWindow
void PersistentTransientWindow::_postHide() {
	activateParent();
}

// Virtual pre-destroy callback, called by TransientWindow before the window
// itself has been destroyed
void PersistentTransientWindow::_preDestroy() {
	// If this window is active, make the parent active instead
	activateParent();
	
	// Disconnect the resize handler callback from the parent widget
	g_signal_handler_disconnect(G_OBJECT(_parent), _parentResizeHandler);
}

gboolean PersistentTransientWindow::_onParentResize(
				GtkWidget* widget, 
				GdkEventWindowState* event, 
				PersistentTransientWindow* self) 
{
	// Check, if the event is of interest
	if ((event->changed_mask & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN)) != 0) {
		// Now let's see what the new state of the main window is
		if ((event->new_window_state & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN)) != 0) {
			// The parent got minimised, minimise the child as well
			minimise(self->getWindow());
		}
		else {
			// Restore the child as the parent is now visible again
			restore(self->getWindow());
		}
	}

	return false;
}

void PersistentTransientWindow::restore(GtkWidget* window) {
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

void PersistentTransientWindow::minimise(GtkWidget* window) {
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

} // namespace gtkutil
