#include "TextureChooser.h"

#include "groupdialog.h"

namespace ui
{

// Construct the dialog

TextureChooser::TextureChooser(GtkWidget* entry)
: _entry(entry),
  _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	GtkWindow* gd = GroupDialog_getWindow();

	gtk_window_set_transient_for(GTK_WINDOW(_widget), gd);
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose texture");

	// Set the default size of the window to slightly smaller than
	// the parent GroupDialog.
	
	gint w, h;
	gtk_window_get_size(gd, &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), gint(w / 1.1), gint(h / 1.1));
	
	// Construct main VBox, and pack in TreeView and buttons panel
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
	// Show all widgets
	gtk_widget_show_all(_widget);
}

// Construct the tree view

GtkWidget* TextureChooser::createTreeView() {
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	GtkWidget* tree = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroll), tree);
	return scroll;
}

// Construct the buttons

GtkWidget* TextureChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_end(GTK_BOX(hbx), gtk_button_new_from_stock(GTK_STOCK_OK), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), gtk_button_new_from_stock(GTK_STOCK_CANCEL), FALSE, FALSE, 0);
	return hbx;
}

} // namespace ui
