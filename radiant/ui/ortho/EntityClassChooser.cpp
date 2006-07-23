#include "EntityClassChooser.h"

#include "mainframe.h"
#include "ieclass.h"
#include "eclasslib.h"

#include "entity.h" // Entity_createFromSelection()

namespace ui
{

// Obtain and display the singleton instance

void EntityClassChooser::displayInstance(const Vector3& point) {
	static EntityClassChooser instance;
	instance.show(point);
}

// Show the dialog

void EntityClassChooser::show(const Vector3& point) {
	_lastPoint = point;
	gtk_widget_show_all(_widget);
}

// Constructor. Creates GTK widgets.

EntityClassChooser::EntityClassChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _treeStore(NULL),
  _selection(NULL),
  _addButton(NULL)
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

	// Set up the TreeModel, and populate it with the list of entity
	// classes by using a local visitor class.
	
	_treeStore = gtk_tree_store_new(1, G_TYPE_STRING);

	struct TreePopulatingVisitor: public EntityClassVisitor {

		// TreeStore to populate
		GtkTreeStore* _store;
		
		// Constructor
		TreePopulatingVisitor(GtkTreeStore* store)
		: _store(store) {}
		
		// Required visit function
		virtual void visit(EntityClass* e) {
			GtkTreeIter iter;
			gtk_tree_store_append(_store, &iter, NULL);
			gtk_tree_store_set(_store, &iter, 0, e->name(), -1);
		}
		
	} visitor(_treeStore);
	
	GlobalEntityClassManager().forEach(visitor);
	
	// Construct the tree view widget with the now-populated model

	GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
	gtk_tree_selection_set_mode(_selection, GTK_SELECTION_BROWSE);
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelectionChanged), this);

	GtkCellRenderer* rend = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col = 
		gtk_tree_view_column_new_with_attributes("Entity name", 
									      		 rend,
									      		 "text", 0,
									      		 NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), col);				
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), FALSE);

	GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrollWin), treeView);
	return scrollWin;
}

// Create the button panel

GtkWidget* EntityClassChooser::createButtonPanel() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	_addButton = gtk_button_new_from_stock(GTK_STOCK_ADD);
	
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);
	g_signal_connect(G_OBJECT(_addButton), "clicked", G_CALLBACK(callbackAdd), this);

	gtk_box_pack_end(GTK_BOX(hbx), _addButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, FALSE, FALSE, 0);
	return hbx;
}

/* GTK CALLBACKS */

void EntityClassChooser::callbackHide(GtkWidget* widget, GdkEvent* ev, EntityClassChooser* self) {
	gtk_widget_hide(self->_widget);
}

void EntityClassChooser::callbackCancel(GtkWidget* widget, EntityClassChooser* self) {
	gtk_widget_hide(self->_widget);
}

void EntityClassChooser::callbackAdd(GtkWidget* widget, EntityClassChooser* self) {

	// Get the selection
	GtkTreeIter iter;
	gtk_tree_selection_get_selected(self->_selection, NULL, &iter);

	// Get the value
	GValue val = {0, 0};
	gtk_tree_model_get_value(GTK_TREE_MODEL(self->_treeStore),
							 &iter,
							 0,
							 &val);

	// Create the entity and hide the dialog
	Entity_createFromSelection(g_value_get_string(&val), self->_lastPoint);
	gtk_widget_hide(self->_widget);							 
}

void EntityClassChooser::callbackSelectionChanged(GtkWidget* widget, EntityClassChooser* self) {
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(self->_selection, NULL, &iter)) {
		// Is a selection
		gtk_widget_set_sensitive(self->_addButton, TRUE);
	}
	else {
		gtk_widget_set_sensitive(self->_addButton, FALSE);
	}
}

} // namespace ui
