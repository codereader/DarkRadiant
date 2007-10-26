#include "EClassTree.h"

#include <gtk/gtk.h>

#include "ieclass.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/IconTextColumn.h"

#include "EClassTreeBuilder.h"

namespace ui {

EClassTree::EClassTree() :
	gtkutil::BlockingTransientWindow(ECLASSTREE_TITLE, GlobalRadiant().getMainWindow())
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Set the default size of the window
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(getWindow()));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);

	gtk_window_set_default_size(GTK_WINDOW(getWindow()), w/2, 2*h/3);
		
	// Create a new tree store for the entityclasses
	_eclassStore = gtk_tree_store_new(
		N_COLUMNS, 
		G_TYPE_STRING,		// name
		GDK_TYPE_PIXBUF		// icon
	);
	
	// Construct an eclass visitor and traverse the 
	EClassTreeBuilder builder(_eclassStore);
	
	// Construct the window's widgets
	populateWindow();
	
	// Enter main loop
	show();
}

void EClassTree::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), _dialogVBox);
	
	GtkWidget* panes = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(_dialogVBox), panes, TRUE, TRUE, 0);
	
	// Pack tree view
	gtk_paned_add1(GTK_PANED(panes), createEClassTreeView());
	
	// Pack spawnarg treeview
	//gtk_paned_add1(GTK_PANED(panes), GTK_WIDGET(_propertyTreeView));
	
	// Pack in dialog buttons
	gtk_box_pack_start(GTK_BOX(_dialogVBox), createButtons(), FALSE, FALSE, 0);
}

GtkWidget* EClassTree::createEClassTreeView() {
	_eclassTreeView = GTK_TREE_VIEW(
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_eclassStore))
	);
	
	// Tree selection
	_eclassSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_eclassTreeView));
	gtk_tree_selection_set_mode(_eclassSelection, GTK_SELECTION_BROWSE);
	//g_signal_connect(G_OBJECT(_eclassSelection), "changed", G_CALLBACK(callbackSelectionChanged), this);
	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_eclassTreeView), TRUE);

	// Pack the columns
	// Single column with icon and name
	GtkTreeViewColumn* col = 
		gtkutil::IconTextColumn("Classname", NAME_COLUMN, ICON_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, NAME_COLUMN);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_eclassTreeView), col);
	
	return gtkutil::ScrolledFrame(GTK_WIDGET(_eclassTreeView));
}

// Lower dialog buttons
GtkWidget* EClassTree::createButtons() {
	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	// Close Button
	GtkWidget* closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(closeButton), "clicked", G_CALLBACK(onClose), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), closeButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(buttonHBox);	
}

// Static command target
void EClassTree::showWindow() {
	// Construct a new instance, this enters the main loop
	EClassTree _tree;
}

void EClassTree::onClose(GtkWidget* button, EClassTree* self) {
	self->destroy();
}

} // namespace ui
