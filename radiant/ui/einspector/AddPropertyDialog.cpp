#include "AddPropertyDialog.h"
#include "PropertyEditorFactory.h"

#include "xmlutil/AttributeNotFoundException.h"
#include "gtkutil/image.h"

#include "groupdialog.h"
#include "qerplugin.h"

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
    
    gint w, h;
	gtk_window_get_size(groupdialog, &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w, h);
    
    // Signals
    g_signal_connect(G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), this);
    
    // Create components
    GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
    gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbx), createButtonsPanel(), FALSE, FALSE, 0);
    
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
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, FALSE, FALSE, 0);
	
	return hbx;
}

// Populate tree view

void AddPropertyDialog::populateTreeView() {
	// Ask the XML registry for the list of properties
	xml::NodeList propNodes = GlobalRadiant().registry().findXPath(PROPERTIES_XPATH);
	
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
		try {
			std::string category = iter->getAttributeValue("category");
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
		catch (xml::AttributeNotFoundException e) {
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

// Show the widgets and block for a selection

const std::string& AddPropertyDialog::showAndBlock() {
	gtk_widget_show_all(_widget);
	gtk_main();
	return _selectedProperty;	
}

// Static method to create and show an instance, and return the chosen
// property to calling function.

std::string AddPropertyDialog::chooseProperty() {
	AddPropertyDialog dialog;
	return dialog.showAndBlock();	
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

void AddPropertyDialog::_onSelectionChanged(GtkWidget* w, AddPropertyDialog* self) {

	// Check for a selection, and retrieve it if it exists
	GtkTreeIter tmpIter;
	GtkTreeModel* model;
	
	if (gtk_tree_selection_get_selected(self->_selection, &model, &tmpIter)) {
        GValue selString = {0, 0};
        gtk_tree_model_get_value(GTK_TREE_MODEL(model), &tmpIter, PROPERTY_NAME_COLUMN, &selString);
        self->_selectedProperty = g_value_get_string(&selString);
        g_value_unset(&selString);
            }
    else {
    	self->_selectedProperty = "";
    }
}

}
