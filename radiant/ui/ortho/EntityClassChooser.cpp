#include "EntityClassChooser.h"

#include "mainframe.h"

namespace ui
{

// Obtain and display the singleton instance

void EntityClassChooser::displayInstance(const Vector3& point) {
	static EntityClassChooser instance;
	instance.show(point);
}

// Show the dialog

void EntityClassChooser::show(const Vector3& point) {
	gtk_widget_show_all(_widget);
}

// Constructor. Creates GTK widgets.

EntityClassChooser::EntityClassChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);

	// Signals
	g_signal_connect(_widget, "delete_event", G_CALLBACK(callbackHide), this);

}

/* GTK CALLBACKS */

void EntityClassChooser::callbackHide(GtkWidget* widget, GdkEvent* ev, EntityClassChooser* self) {
	gtk_widget_hide(widget);
}

} // namespace ui
