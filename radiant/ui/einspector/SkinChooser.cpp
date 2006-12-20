#include "SkinChooser.h"

#include "modelskin.h"
#include "groupdialog.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"

#include <gtk/gtk.h>

namespace ui
{

/* CONSTANTS */

namespace {
	
	// Tree column enum
	enum {
		DISPLAYNAME_COL,
		FULLNAME_COL,
		N_COLUMNS
	};
	
}

// Constructor
SkinChooser::SkinChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _lastSkin("")
{
	// Set up window
	GtkWindow* gd = GroupDialog_getWindow();

	gtk_window_set_transient_for(GTK_WINDOW(_widget), gd);
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose skin");

	// Set the default size of the window
	gint w, h;
	gtk_window_get_size(gd, &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w, h);
	
	// Vbox contains treeview and buttons panel
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
}

// Create the TreeView
GtkWidget* SkinChooser::createTreeView() {
	
	// Create the treestore
	_treeStore = gtk_tree_store_new(N_COLUMNS, 
  									G_TYPE_STRING, 
  									G_TYPE_STRING);
  									
	// Create the tree view
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);
	
	// Single column to display the skin name
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), 
								gtkutil::TextColumn("Skin", DISPLAYNAME_COL));
	
	// Pack treeview into a ScrolledFrame and return
	return gtkutil::ScrolledFrame(_treeView);
}

// Create the buttons panel
GtkWidget* SkinChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);
	
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	g_signal_connect(G_OBJECT(okButton), "clicked", 
					 G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", 
					 G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);	
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
					   
	return gtkutil::RightAlignment(hbx);	
}

// Show the dialog and block for a selection
std::string SkinChooser::showAndBlock(const std::string& model,
									  const std::string& prev)
{
	
	// Set the model and previous skin, then populate the skins
	_model = model;
	_prevSkin = prev;
	populateSkins();
	
	// Show the dialog
	gtk_widget_show_all(_widget);
	
	// Enter main loop and block
	gtk_main();
	
	// Hide the dialog and return the selection
	gtk_widget_hide(_widget);
	return _lastSkin;
}

// Populate the list of skins
void SkinChooser::populateSkins() {
	
	// Clear the treestore
	gtk_tree_store_clear(_treeStore);
	
	// Add "default" option to select no skin
	GtkTreeIter iter;
	gtk_tree_store_append(_treeStore, &iter, NULL);
	gtk_tree_store_set(_treeStore, &iter, 
					   DISPLAYNAME_COL, "*Default", 
					   FULLNAME_COL, "",
					   -1); 		
	
	// Get the list of skins for the model
	const StringList& skins = GlobalModelSkinCache().getSkinsForModel(_model);
		
	// Add each skin to the tree view
	for (StringList::const_iterator i = skins.begin();
		 i != skins.end();
		 ++i)
	{
		GtkTreeIter iter;
		gtk_tree_store_append(_treeStore, &iter, NULL);
		gtk_tree_store_set(_treeStore, &iter, 
						   DISPLAYNAME_COL, i->c_str(), 
						   FULLNAME_COL, i->c_str(),
						   -1); 		
	}
}

// Static method to display singleton instance and choose a skin
std::string SkinChooser::chooseSkin(const std::string& model,
									const std::string& prev) 
{
	
	// The static instance
	static SkinChooser _instance;
	
	// Show and block the instance, returning the selected skin
	return _instance.showAndBlock(model, prev);	
}

/* GTK CALLBACKS */

void SkinChooser::_onOK(GtkWidget* widget, SkinChooser* self) {

	// Get the selection
	GtkTreeSelection* sel = 
		gtk_tree_view_get_selection(GTK_TREE_VIEW(self->_treeView));

	// Get the selected skin, and set the lastskin variable for return
	GtkTreeIter iter;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		self->_lastSkin = gtkutil::TreeModel::getString(model, 
														&iter, 
														FULLNAME_COL);
	}
	else {
		// Nothing selected, return empty string
		self->_lastSkin = "";
	}
	
	// Exit main loop
	gtk_main_quit();
}

void SkinChooser::_onCancel(GtkWidget* widget, SkinChooser* self) {
	// Clear the last skin and quit the main loop
	self->_lastSkin = self->_prevSkin;
	gtk_main_quit();	
}

}
