#include "ComponentsDialog.h"

#include <gtk/gtk.h>

namespace objectives
{

/* CONSTANTS */

namespace {

	const char* DIALOG_TITLE = "Edit conditions";
	
}

// Main constructor
ComponentsDialog::ComponentsDialog(GtkWindow* parent)
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Set up window
	gtk_window_set_transient_for(GTK_WINDOW(_widget), parent);
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), DIALOG_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);

	// Set up delete event
	g_signal_connect(
		G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), this);
}

// Show dialog and block
void ComponentsDialog::showAndBlock() {
	gtk_widget_show_all(_widget);
	gtk_main(); // recursive main loop	
}

/* GTK CALLBACKS */

void ComponentsDialog::_onDelete(GtkWidget* w, ComponentsDialog* self) {
	
	// Exit recursive main loop
	gtk_main_quit();
}

}
