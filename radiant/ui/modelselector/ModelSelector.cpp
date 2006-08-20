#include "ModelSelector.h"

#include "mainframe.h"

#include <iostream>

namespace ui
{

// CONSTANTS

namespace {
	
	const char* MODELSELECTOR_TITLE = "Choose model";
	
}

// Constructor.

ModelSelector::ModelSelector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Window properties
	
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), MODELSELECTOR_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);

	// Set the default size of the window
	
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);
	
	gtk_window_set_default_size(GTK_WINDOW(_widget), w / 2, gint(h / 1.5));

	// Signals
	
	g_signal_connect(G_OBJECT(_widget), "delete_event", G_CALLBACK(callbackHide), this);
	
	// Main window contains a VBox
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
}

// Show the dialog and enter recursive main loop

std::string ModelSelector::showAndBlock() {
	gtk_widget_show_all(_widget);
	gtk_main(); // recursive main loop
	return "models/test";
}

// Static function to display the instance, and return the selected
// model to the calling function

std::string ModelSelector::chooseModel() {
	static ModelSelector _selector;
	return _selector.showAndBlock();
}

// Helper function to create the TreeView

GtkWidget* ModelSelector::createTreeView() {
	return gtk_tree_view_new();
}

/* GTK CALLBACKS */

void ModelSelector::callbackHide(GtkWidget* widget, GdkEvent* ev, ModelSelector* self) {
	gtk_widget_hide(self->_widget);
	gtk_main_quit(); // exit recursive main loop
}

}
