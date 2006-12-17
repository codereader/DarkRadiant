#include "EntityClassChooser.h"

#include "mainframe.h"
#include "ieclass.h"
#include "ieclass.h"
#include "iregistry.h"
#include "gtkutil/dialog.h"
#include "gtkutil/image.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"

#include "entity.h" // Entity_createFromSelection()

#include <ext/hash_map>
#include <boost/functional/hash/hash.hpp>

namespace ui
{

// CONSTANTS

namespace {
	
	const char* FOLDER_ICON = "folder16.png";
	const char* ENTITY_ICON = "cmenu_add_entity.png";

	// Registry XPath to lookup key that specifies the display folder
	const char* FOLDER_KEY_PATH = "game/entityChooser/displayFolderKey";
	
	// Tree column enum
	enum {
		NAME_COLUMN,
		ICON_COLUMN,
		DIR_FLAG_COLUMN,
		N_COLUMNS
	};
	
}

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

	// Create GUI elements and pack into main VBox
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createUsagePanel(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtonPanel(), FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);

	// Signals
	g_signal_connect(_widget, "delete_event", G_CALLBACK(callbackHide), this);

}

// Create the tree view

GtkWidget* EntityClassChooser::createTreeView() {

	// Set up the TreeModel, and populate it with the list of entity
	// classes by using a local visitor class.
	
	_treeStore = gtk_tree_store_new(N_COLUMNS, 
									G_TYPE_STRING,		// name
									GDK_TYPE_PIXBUF,	// icon
									G_TYPE_BOOLEAN);	// directory flag

	class TreePopulatingVisitor: public EntityClassVisitor {

		// Map between string directory names and their corresponding Iters
		typedef __gnu_cxx::hash_map<std::string, GtkTreeIter*, boost::hash<std::string> > DirIterMap;
		DirIterMap _dirIterMap;

		// TreeStore to populate
		GtkTreeStore* _store;
		
		// Key that specifies the display folder
		std::string _folderKey;
		
	public:

		// Constructor
		TreePopulatingVisitor(GtkTreeStore* store)
		: _store(store),
		  _folderKey(GlobalRegistry().get(FOLDER_KEY_PATH))
		{}

		// Recursive folder add function
		GtkTreeIter* addRecursive(const std::string& pathName) {

			// Lookup pathname in map, and return the GtkTreeIter* if it is
			// found
			DirIterMap::iterator iTemp = _dirIterMap.find(pathName);
			if (iTemp != _dirIterMap.end()) { // found in map
				return iTemp->second;
			}
			
			// Split the path into "this directory" and the parent path
			unsigned int slashPos = pathName.rfind("/");
			const std::string parentPath = pathName.substr(0, slashPos);
			const std::string thisDir = pathName.substr(slashPos + 1);

			// Recursively add parent path
			GtkTreeIter* parIter = NULL;
			if (slashPos != std::string::npos)
				parIter = addRecursive(parentPath);

			// Now add "this directory" as a child, saving the iter in the map
			// and returning it.
			GtkTreeIter iter;
			gtk_tree_store_append(_store, &iter, parIter);
			gtk_tree_store_set(_store, &iter, 
							   NAME_COLUMN, thisDir.c_str(),
							   ICON_COLUMN, gtkutil::getLocalPixbuf(FOLDER_ICON),
							   DIR_FLAG_COLUMN, TRUE,
							   -1);
			GtkTreeIter* dynIter = gtk_tree_iter_copy(&iter); // get a heap-allocated iter
			
			// Cache the dynamic iter and return it
			_dirIterMap[pathName] = dynIter;
			return dynIter;
			
		}

		// Add parent folder
		GtkTreeIter* addDisplayFolder(IEntityClass* e) {

			// Get the parent folder from the entity class. If it is not
			// present, return NULL
			std::string parentFolder = e->getValueForKey(_folderKey);
			if (parentFolder.size() == 0)
				return NULL;
				
			// Call the recursive function to add the folder
			return addRecursive(parentFolder);
		}

		// Required visit function
		virtual void visit(IEntityClass* e) {

			// Recursively create the folder to put this EntityClass in,
			// depending on the value of the DISPLAY_FOLDER_KEY. This may return
			// NULL if the key is unset, in which case we add the entity at
			// the top level.
			GtkTreeIter* parIter = addDisplayFolder(e);
			
			// Add the new class under the parent folder
			GtkTreeIter iter;
			gtk_tree_store_append(_store, &iter, parIter);
			gtk_tree_store_set(_store, &iter, 
							   NAME_COLUMN, e->getName().c_str(), 
							   ICON_COLUMN, gtkutil::getLocalPixbuf(ENTITY_ICON),
							   DIR_FLAG_COLUMN, FALSE,
							   -1);
		}
		
		
	} visitor(_treeStore);
	
	GlobalEntityClassManager().forEach(visitor);
	
	// Construct the tree view widget with the now-populated model

	GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), FALSE);

	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
	gtk_tree_selection_set_mode(_selection, GTK_SELECTION_BROWSE);
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelectionChanged), this);

	// Single column with icon and name
	GtkTreeViewColumn* col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_spacing(col, 3);

	GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(col, pixRenderer, "pixbuf", ICON_COLUMN, NULL);

	GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, textRenderer, FALSE);
	gtk_tree_view_column_set_attributes(col, textRenderer, "text", NAME_COLUMN, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), col);				

	// Pack treeview into a scrolled frame and return
	return gtkutil::ScrolledFrame(treeView);
}

// Create the entity usage information panel
GtkWidget* EntityClassChooser::createUsagePanel() {

	// Create a GtkTextView
	_usageTextView = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(_usageTextView), GTK_WRAP_WORD);

	return gtkutil::ScrolledFrame(_usageTextView);	
}

// Create the button panel
GtkWidget* EntityClassChooser::createButtonPanel() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	_addButton = gtk_button_new_from_stock(GTK_STOCK_ADD);
	
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);
	g_signal_connect(G_OBJECT(_addButton), "clicked", G_CALLBACK(callbackAdd), this);

	gtk_box_pack_end(GTK_BOX(hbx), _addButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
	return gtkutil::RightAlignment(hbx);
}

// Update the usage information
void EntityClassChooser::updateUsageInfo(const std::string& eclass) {

	// Lookup the IEntityClass instance
	IEntityClass* e = GlobalEntityClassManager().findOrInsert(eclass.c_str(), true);	

	// Set the usage panel to the IEntityClass' usage information string
	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_usageTextView));
	gtk_text_buffer_set_text(buf, e->getValueForKey("editor_usage").c_str(), -1);
}

/* GTK CALLBACKS */

void EntityClassChooser::callbackHide(GtkWidget* widget, GdkEvent* ev, EntityClassChooser* self) {
	gtk_widget_hide(self->_widget);
}

void EntityClassChooser::callbackCancel(GtkWidget* widget, EntityClassChooser* self) {
	gtk_widget_hide(self->_widget);
}

void EntityClassChooser::callbackAdd(GtkWidget* widget, EntityClassChooser* self) {

	// Get the selection. There must be a selection because the add button is
	// insensitive if there is not.
	GtkTreeIter iter;
	GtkTreeModel* model;
	gtk_tree_selection_get_selected(self->_selection, &model, &iter);

	// Create the entity and hide the dialog. We might get an 
	// EntityCreationException if the wrong number of brushes is selected.
	try {
		// Get the entity classname
		std::string cName = 
			gtkutil::TreeModel::getString(model, &iter, NAME_COLUMN);
		// Create the entity and hide the dialog
		Entity_createFromSelection(cName.c_str(), self->_lastPoint);
		gtk_widget_hide(self->_widget);
	}
	catch (EntityCreationException e) {
		gtkutil::errorDialog(e.what());
	}
}

void EntityClassChooser::callbackSelectionChanged(GtkWidget* widget, EntityClassChooser* self) {

	// Prepare to check for a selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	
	// Add button is enabled if there is a selection and it is not a folder.
	if (gtk_tree_selection_get_selected(self->_selection, &model, &iter)
		&& !gtkutil::TreeModel::getBoolean(model, &iter, DIR_FLAG_COLUMN)) 
	{
		// Make the Add button active 
		gtk_widget_set_sensitive(self->_addButton, TRUE);

		// Set the panel text with the usage information
		self->updateUsageInfo(
			gtkutil::TreeModel::getString(model, &iter, NAME_COLUMN));
	}
	else {
		gtk_widget_set_sensitive(self->_addButton, FALSE);
	}
}

} // namespace ui
