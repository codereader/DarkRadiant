#include "AddPropertyDialog.h"
#include "PropertyEditorFactory.h"

#include "gtkutil/image.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"

#include "groupdialog.h"
#include "qerplugin.h"
#include "iregistry.h"

#include <gtk/gtkwindow.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>

#include <map>

namespace ui
{
	
namespace {
	
	// Tree columns
	enum {
		DISPLAY_NAME_COLUMN,
		PROPERTY_NAME_COLUMN,
		ICON_COLUMN,
		N_COLUMNS
	};
	
	// CONSTANTS
	const char* ADDPROPERTY_TITLE = "Add property";
	const char* PROPERTIES_XPATH = "game/entityInspector//property";
	const char* FOLDER_ICON = "folder16.png";
}

// Constructor creates GTK widgets

AddPropertyDialog::AddPropertyDialog()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Window properties
	GtkWindow* groupdialog = GroupDialog_getWindow();
	
	gtk_window_set_transient_for(GTK_WINDOW(_widget), groupdialog);
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), ADDPROPERTY_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    
    // Set size of dialog
    gint w, h;
	gtk_window_get_size(groupdialog, &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w, h);
    
    // Signals
    g_signal_connect(G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), this);
    
    // Create components
    GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbx), createButtonsPanel(), FALSE, FALSE, 0);
    
    
    // Pack into window
    gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
    gtk_container_add(GTK_CONTAINER(_widget), vbx);
    
    // Populate the tree view with properties
    populateTreeView();
}

// Construct the tree view

GtkWidget* AddPropertyDialog::createTreeView() {
	// Set up the tree store
	_treeStore = gtk_tree_store_new(N_COLUMNS,
									G_TYPE_STRING, // display name
									G_TYPE_STRING, // property name
									GDK_TYPE_PIXBUF); // icon
	// Create tree view
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);

	// Connect up selection changed callback
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(_onSelectionChanged), this);
	
	// Display name column with icon
	GtkTreeViewColumn* nameCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_spacing(nameCol, 3);

	GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(nameCol, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(nameCol, pixRenderer, "pixbuf", ICON_COLUMN, NULL);

	GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(nameCol, textRenderer, FALSE);
    gtk_tree_view_column_set_attributes(nameCol,
                                        textRenderer,
                                        "text",
                                        DISPLAY_NAME_COLUMN,
                                        NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), nameCol);                                                                        

	// Model owned by view
	g_object_unref(G_OBJECT(_treeStore));
	
	// Pack into scrolled window and frame, and return
	
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), _treeView);
	
	GtkWidget* frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), scroll);
	
	return frame;
}

// Construct the buttons panel

GtkWidget* AddPropertyDialog::createButtonsPanel() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);
	
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(hbx);	
}

// Populate tree view

void AddPropertyDialog::populateTreeView() {
	// Ask the XML registry for the list of properties
	xml::NodeList propNodes = GlobalRegistry().findXPath(PROPERTIES_XPATH);
	
	// Cache of property categories to GtkTreeIters, to allow properties
	// to be parented to top-level categories
	typedef std::map<std::string, GtkTreeIter*> CategoryMap;
	CategoryMap categories;
	
	// Add each property to the tree view
	for (xml::NodeList::const_iterator iter = propNodes.begin();
		 iter != propNodes.end();
		 ++iter)
	{
		GtkTreeIter t;

		// If this property has a category, look up the top-level parent iter
		// or add it if necessary.
		std::string category = iter->getAttributeValue("category");
		if (!category.empty()) {
			CategoryMap::iterator mIter = categories.find(category);
			
			if (mIter == categories.end()) {
				// Not found, add to treestore
				GtkTreeIter tIter;
				gtk_tree_store_append(_treeStore, &tIter, NULL);
				gtk_tree_store_set(_treeStore, &tIter,
								   DISPLAY_NAME_COLUMN, category.c_str(),
								   PROPERTY_NAME_COLUMN, "",
								   ICON_COLUMN, gtkutil::getLocalPixbuf(FOLDER_ICON),
								   -1);
				// Add to map
				mIter = categories.insert(CategoryMap::value_type(category, gtk_tree_iter_copy(&tIter))).first;
			}
			
			// Category sorted, add this property below it
			gtk_tree_store_append(_treeStore, &t, mIter->second);
		}
		else {
			// No category, add at toplevel
			gtk_tree_store_append(_treeStore, &t, NULL);
		}
		
		std::string name = iter->getAttributeValue("name");
		std::string type = iter->getAttributeValue("type");
		
		gtk_tree_store_set(_treeStore, &t,
						   DISPLAY_NAME_COLUMN, name.c_str(),
						   PROPERTY_NAME_COLUMN, name.c_str(),
						   ICON_COLUMN, PropertyEditorFactory::getPixbufFor(type),
						   -1);
	}
}

// Static method to create and show an instance, and return the chosen
// property to calling function.

std::string AddPropertyDialog::chooseProperty() {
	
	// Construct a dialog and show the main widget
	AddPropertyDialog dialog;
	gtk_widget_show_all(dialog._widget);
	
	// Block for a selection
	gtk_main();
	
	// Return the last selection to calling process
	return dialog._selectedProperty;
}

/* GTK CALLBACKS */

void AddPropertyDialog::_onDelete(GtkWidget* w, GdkEvent* e, AddPropertyDialog* self) {
	self->_selectedProperty = "";
	gtk_widget_destroy(self->_widget);
	gtk_main_quit(); // exit recursive main loop	
}

void AddPropertyDialog::_onOK(GtkWidget* w, AddPropertyDialog* self) {
	gtk_widget_destroy(self->_widget);
	gtk_main_quit(); // exit recursive main loop	
}

void AddPropertyDialog::_onCancel(GtkWidget* w, AddPropertyDialog* self) {
	self->_selectedProperty = "";
	gtk_widget_destroy(self->_widget);
	gtk_main_quit(); // exit recursive main loop	
}

void AddPropertyDialog::_onSelectionChanged(GtkWidget* w, 
											AddPropertyDialog* self) 
{
	self->_selectedProperty = 
		gtkutil::TreeModel::getSelectedString(self->_selection,
											  PROPERTY_NAME_COLUMN);
}

}
