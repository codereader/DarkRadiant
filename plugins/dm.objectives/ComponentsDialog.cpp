#include "ComponentsDialog.h"
#include "Objective.h"

#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TextColumn.h"

#include <gtk/gtk.h>

namespace objectives
{

/* CONSTANTS */

namespace {

	const char* DIALOG_TITLE = "Edit conditions";
	
	// Names of component types (matches the ComponentType enum)
	const char* COMPONENT_TYPE_NAMES[] = {
		"Kill", "Knockout", "AI finds item", "AI finds body", "AI alert",
		"Inventory item", "Item in location", "Custom script", "Custom clocked",
		"Item in info_location", "Item distance from origin"
	};
}

// Main constructor
ComponentsDialog::ComponentsDialog(GtkWindow* parent, Objective& objective)
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _objective(objective),
  _componentList(gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING))
{
	// Set up window
	gtk_window_set_transient_for(GTK_WINDOW(_widget), parent);
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), DIALOG_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);

	// Set up delete event
	g_signal_connect(
		G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), this);
		
	// Dialog contains list view, edit panel and buttons
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbx), createListView(), TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
}

// Create list view
GtkWidget* ComponentsDialog::createListView() {
	
	GtkWidget* tv = 
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_componentList));

	// Number column
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn("#", 0, false));
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv), gtkutil::TextColumn("Type", 1, false));

	// Populate the list store with components from the objective
	Objective::ComponentMap& components = _objective.components;
	for (Objective::ComponentMap::const_iterator i = components.begin();
		 i != components.end();
		 ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(_componentList, &iter);
		gtk_list_store_set(_componentList, &iter, 
						   0, i->first, 
						   1, i->second.type.c_str(),
						   -1);	
	}
	

	return gtkutil::ScrolledFrame(tv);	
}

// Create buttons
GtkWidget* ComponentsDialog::createButtons() {
	
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
//	g_signal_connect(
//		G_OBJECT(okButton), "clicked", G_CALLBACK(_onOK), this);
//	g_signal_connect(
//		G_OBJECT(cancelButton), "clicked", G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
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
