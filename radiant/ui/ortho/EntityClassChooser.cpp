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
	gtk_window_set_title(GTK_WINDOW(_widget), ECLASS_CHOOSER_TITLE);

	// Set the default size of the window
	
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);
	
	gtk_window_set_default_size(GTK_WINDOW(_widget), w / 3, h / 2);

	// Main window contains a VBox which divides the area into two. The bottom
	// part contains the buttons, while the top part contains a single tree
	// view containing the complete Entity Class tree.
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtonPanel(), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);

	// Signals
	g_signal_connect(_widget, "delete_event", G_CALLBACK(callbackHide), this);

}

// Create the tree view

GtkWidget* EntityClassChooser::createTreeView() {
	GtkWidget* treeView = gtk_tree_view_new();
	return treeView;
}

// Create the button panel

GtkWidget* EntityClassChooser::createButtonPanel() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_end(GTK_BOX(hbx), gtk_button_new_from_stock(GTK_STOCK_ADD), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), gtk_button_new_from_stock(GTK_STOCK_CANCEL), FALSE, FALSE, 0);
	return hbx;
}

/* GTK CALLBACKS */

void EntityClassChooser::callbackHide(GtkWidget* widget, GdkEvent* ev, EntityClassChooser* self) {
	gtk_widget_hide(widget);
}

} // namespace ui
