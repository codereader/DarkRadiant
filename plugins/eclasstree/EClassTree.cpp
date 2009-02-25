#include "EClassTree.h"

#include <gtk/gtk.h>

#include "ieclass.h"
#include "selectionlib.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TreeModel.h"

#include "EClassTreeBuilder.h"

namespace ui {

	namespace {
		// TreeView column numbers
	    enum {
	        PROPERTY_NAME_COLUMN,
	        PROPERTY_VALUE_COLUMN,
	        PROPERTY_TEXT_COLOUR_COLUMN,
	        PROPERTY_INHERITED_FLAG_COLUMN,
	        NUM_PROPERTY_COLUMNS
	    };
	}

EClassTree::EClassTree() :
	gtkutil::BlockingTransientWindow(ECLASSTREE_TITLE, GlobalRadiant().getMainWindow())
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
		
	// Create a new tree store for the entityclasses
	_eclassStore = gtk_tree_store_new(
		N_COLUMNS, 
		G_TYPE_STRING,		// name
		GDK_TYPE_PIXBUF		// icon
	);
	
	// Construct an eclass visitor and traverse the entity classes
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
	
	GtkWidget* paned = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(_dialogVBox), paned, TRUE, TRUE, 0);
	
	// Pack tree view
	gtk_paned_add1(GTK_PANED(paned), createEClassTreeView());
	
	// Pack spawnarg treeview
	gtk_paned_add2(GTK_PANED(paned), GTK_WIDGET(createPropertyTreeView()));
	
	// Pack in dialog buttons
	gtk_box_pack_start(GTK_BOX(_dialogVBox), createButtons(), FALSE, FALSE, 0);
	
	// Set the default size of the window
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(getWindow()));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);

	gtk_window_set_default_size(GTK_WINDOW(getWindow()), 2*w/3, 2*h/3);
	gtk_paned_set_position(GTK_PANED(paned), w/4);
}

GtkWidget* EClassTree::createEClassTreeView() {
	_eclassView = GTK_TREE_VIEW(
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_eclassStore))
	);

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(_eclassView, gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);
	
	// Tree selection
	_eclassSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_eclassView));
	gtk_tree_selection_set_mode(_eclassSelection, GTK_SELECTION_BROWSE);
	g_signal_connect(G_OBJECT(_eclassSelection), "changed", G_CALLBACK(onSelectionChanged), this);
	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_eclassView), TRUE);

	// Pack the columns
	// Single column with icon and name
	GtkTreeViewColumn* col = 
		gtkutil::IconTextColumn("Classname", NAME_COLUMN, ICON_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, NAME_COLUMN);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_eclassView), col);
	
	return gtkutil::ScrolledFrame(GTK_WIDGET(_eclassView));
}

GtkWidget* EClassTree::createPropertyTreeView() {
	// Initialise the instance TreeStore
	_propertyStore = gtk_list_store_new(NUM_PROPERTY_COLUMNS, 
    							    G_TYPE_STRING, // property
    							    G_TYPE_STRING, // value
                                    G_TYPE_STRING, // text colour
    							    G_TYPE_STRING); // inherited flag
    
    // Create the TreeView widget and link it to the model
	_propertyView = GTK_TREE_VIEW(gtk_tree_view_new_with_model(
		GTK_TREE_MODEL(_propertyStore))
	);

    // Create the Property column
    GtkTreeViewColumn* nameCol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(nameCol, "Property");
	gtk_tree_view_column_set_sizing(nameCol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(nameCol, 3);

    GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(nameCol, textRenderer, FALSE);
    gtk_tree_view_column_set_attributes(nameCol, textRenderer,
                                        "text", PROPERTY_NAME_COLUMN,
                                        "foreground", PROPERTY_TEXT_COLOUR_COLUMN,
                                        NULL);

    //GtkTreeViewColumn* nameCol = gtkutil::TextColumn("Property", PROPERTY_NAME_COLUMN);
	gtk_tree_view_column_set_sort_column_id(nameCol, PROPERTY_NAME_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_propertyView), nameCol);                                                                        

	// Create the value column
    GtkTreeViewColumn* valCol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(valCol, "Value");
	gtk_tree_view_column_set_sizing(valCol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    GtkCellRenderer* valRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(valCol, valRenderer, TRUE);
    gtk_tree_view_column_set_attributes(valCol, valRenderer, 
    									"text", PROPERTY_VALUE_COLUMN, 
    									"foreground", PROPERTY_TEXT_COLOUR_COLUMN,
    									NULL);

	gtk_tree_view_column_set_sort_column_id(valCol, PROPERTY_VALUE_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_propertyView), valCol);
    
    return gtkutil::ScrolledFrame(GTK_WIDGET(_propertyView));
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

void EClassTree::updatePropertyView(const std::string& eclassName) {
	// Clear the existing list
	gtk_list_store_clear(_propertyStore);
	
	IEntityClassPtr eclass = GlobalEntityClassManager().findClass(eclassName);
	if (eclass == NULL) {
		return;
	}
	
	class ListStorePopulator :
		public EntityClassAttributeVisitor
	{
		GtkListStore* _listStore;
	public:
		ListStorePopulator(GtkListStore* targetStore) :
			_listStore(targetStore)
		{}
		
		virtual void visit(const EntityClassAttribute& attr) {
			// Append the details to the treestore
			GtkTreeIter iter;
			gtk_list_store_append(_listStore, &iter);
			gtk_list_store_set(
				_listStore, &iter,
				PROPERTY_NAME_COLUMN, attr.name.c_str(),
				PROPERTY_VALUE_COLUMN, attr.value.c_str(),
				PROPERTY_TEXT_COLOUR_COLUMN, attr.inherited ? "#666666" : "black",
				PROPERTY_INHERITED_FLAG_COLUMN, attr.inherited ? "1" : "0",
				-1
			);
		}
	};
	
	ListStorePopulator populator(_propertyStore);
	eclass->forEachClassAttribute(populator, true);
}

void EClassTree::_preShow() {
	// Do we have anything selected
	if (GlobalSelectionSystem().countSelected() == 0) {
		return;
	}

	// Get the last selected node and check if it's an entity
	scene::INodePtr lastSelected = GlobalSelectionSystem().ultimateSelected();

	Entity* entity = Node_getEntity(lastSelected);

	if (entity != NULL) {
		// There is an entity selected, extract the classname
		std::string classname = entity->getKeyValue("classname");

		// Construct a finder
		gtkutil::TreeModel::SelectionFinder finder(classname, NAME_COLUMN);

		// Traverse the model and find the name
		gtk_tree_model_foreach(
			GTK_TREE_MODEL(_eclassStore), 
			gtkutil::TreeModel::SelectionFinder::forEach, &finder);
		
		// Select the element, if something was found
		GtkTreePath* path = finder.getPath();
		if (path) {
			// Expand the treeview to display the target row
			gtk_tree_view_expand_to_path(_eclassView, path);
			// Highlight the target row
			gtk_tree_view_set_cursor(_eclassView, path, NULL, false);
			// Make the selected row visible 
			gtk_tree_view_scroll_to_cell(_eclassView, path, NULL, true, 0.3f, 0.0f);
		}
	}
}

// Static command target
void EClassTree::showWindow(const cmd::ArgumentList& args) {
	// Construct a new instance, this enters the main loop
	EClassTree _tree;
}

void EClassTree::onClose(GtkWidget* button, EClassTree* self) {
	self->destroy();
}

void EClassTree::onSelectionChanged(GtkWidget* widget, EClassTree* self) {
	// Prepare to check for a selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	
	// Add button is enabled if there is a selection and it is not a folder.
	if (gtk_tree_selection_get_selected(self->_eclassSelection, &model, &iter)) 
	{
		gtk_widget_set_sensitive(GTK_WIDGET(self->_propertyView), TRUE);
		
		// Set the panel text with the usage information
		self->updatePropertyView(
			gtkutil::TreeModel::getString(model, &iter, NAME_COLUMN));
	}
	else {
		gtk_widget_set_sensitive(GTK_WIDGET(self->_propertyView), FALSE);
	}
}

} // namespace ui
