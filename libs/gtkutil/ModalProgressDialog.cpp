#include "ModalProgressDialog.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkmain.h>

namespace gtkutil {

// Main constructor
ModalProgressDialog::ModalProgressDialog(GtkWindow* parent, const std::string& title)
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _label(gtk_label_new("")),
  _progressBar(gtk_progress_bar_new())
{
  	// Window properties
	gtk_window_set_transient_for(GTK_WINDOW(_widget), parent);
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), title.c_str());
	gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_default_size(GTK_WINDOW(_widget), 360, 80);

	g_signal_connect(G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), NULL);

	// Create a vbox
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);

	// Pack a progress bar into the window
	gtk_box_pack_start(GTK_BOX(vbx), _progressBar, FALSE, FALSE, 0);

	// Pack the label into the window
	gtk_box_pack_start(GTK_BOX(vbx), _label, TRUE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
	
	// Show the window
	gtk_widget_show_all(_widget);
	handleEvents();
}

// Set the label text
void ModalProgressDialog::setText(const std::string& text) {

	// Set the text
	gtk_label_set_markup(GTK_LABEL(_label), text.c_str());
	
	// Pulse the progress bar
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(_progressBar));
	
	// Handle GTK events to make changes visible
	handleEvents();
}

// Handle GTK events
void ModalProgressDialog::handleEvents() {
	while (gtk_events_pending())
		gtk_main_iteration();
}

} // namespace gtkutil
