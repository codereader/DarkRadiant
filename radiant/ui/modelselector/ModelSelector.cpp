#include "ModelSelector.h"

#include "mainframe.h"
#include "gtkutil/image.h"

#include "ifilesystem.h"
#include "modelskin.h"

#include <iostream>
#include <vector>
#include <ext/hash_map>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ui
{

// CONSTANTS

namespace {
	
	const char* MODELSELECTOR_TITLE = "Choose model";

	// Treestore enum
	enum {
		NAME_COLUMN,		// e.g. "chair1.lwo"
		FULLNAME_COLUMN,	// e.g. "models/darkmod/props/chair1.lwo"
		IMAGE_COLUMN,		// icon to display
		N_COLUMNS
	};
	
}

// Constructor.

ModelSelector::ModelSelector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _treeStore(gtk_tree_store_new(N_COLUMNS, 
  								G_TYPE_STRING,
  								G_TYPE_STRING,
  								GDK_TYPE_PIXBUF))
{
	// Window properties
	
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), MODELSELECTOR_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);

	// Set the default size of the window
	
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);
	
	gtk_window_set_default_size(GTK_WINDOW(_widget), w / 2, gint(h / 1.5));

	// Signals
	
	g_signal_connect(G_OBJECT(_widget), "delete_event", G_CALLBACK(callbackHide), this);
	
	// Main window contains a VBox
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
}

// Show the dialog and enter recursive main loop

std::string ModelSelector::showAndBlock() {
	gtk_widget_show_all(_widget);
	gtk_main(); // recursive main loop
	return "models/test";
}

// Static function to display the instance, and return the selected
// model to the calling function

std::string ModelSelector::chooseModel() {
	static ModelSelector _selector;
	return _selector.showAndBlock();
}

// File-local functor object to retrieve model names from global VFS

namespace {

	struct ModelFileFunctor {
	
		typedef const char* first_argument_type;
		
		// Tree store to populate

		GtkTreeStore* _store;

		// Map between model directory names (e.g. "models/darkmod/architecture") and
		// a GtkTreeIter pointing to the equivalent row in the TreeModel. Subsequent
		// modelpaths with this directory will be added as children of this iter.

		typedef __gnu_cxx::hash_map<std::string, GtkTreeIter*, boost::hash<std::string> > DirIterMap;
		DirIterMap _dirIterMap;
		
		// Constructor
		
		ModelFileFunctor(GtkTreeStore* store)
		: _store(store) {}
		
		// Destructor. The GtkTreeIters are dynamically allocated so we must free them
		
		~ModelFileFunctor() {
			for (DirIterMap::iterator i = _dirIterMap.begin();
					i != _dirIterMap.end();
						++i) 
			{
				gtk_tree_iter_free(i->second);
			}
		}
		
		// Recursive function to add a given model path ("models/darkmod/something/model.lwo")
		// to its correct place in the tree. This is done by maintaining a cache of 
		// directory nodes ("models/darkmod/something", "models/darkmod") against 
		// GtkIters that point to the corresponding row in the tree model. On each call,
		// the parent node is recursively calculated, and the node provided as an argument
		// added as a child.
		
		GtkTreeIter* addRecursive(const std::string& dirPath) {
			
			// We first try to lookup the directory name in the map. Return it
			// if it exists, otherwise recursively obtain the parent of this directory name,
			// and add this directory as a child in the tree model. We also add this
			// directory to the map for future lookups.
			
			DirIterMap::iterator iTemp = _dirIterMap.find(dirPath);
			if (iTemp != _dirIterMap.end()) { // found in map
				return iTemp->second;
			}
			else { 

				// Perform the search for final "/" which identifies the parent
				// of this directory, and call recursively. If there is no slash, we
				// are looking at a toplevel directory in which case the parent is
				// NULL.
				unsigned int slashPos = dirPath.rfind("/");
				GtkTreeIter* parIter = NULL;
				
				if (slashPos != std::string::npos) {
					parIter = addRecursive(dirPath.substr(0, slashPos));
				}

				// Add this directory to the treemodel. For the displayed tree, we
				// want the last component of the directory name, not the entire path
				// at each node.

				// Decide which image to use, based on the file extension (or the folder
				// image if there is no extension).
				std::string imgPath = "folder16.png";
				if (boost::algorithm::iends_with(dirPath, ".lwo"))
					imgPath = "model16red.png";
				else if (boost::algorithm::iends_with(dirPath, ".ase"))
					imgPath = "model16green.png";

				// If no slash found, want the whole string. Set slashPos to -1 to 
				// offset the +1 we give it later to skip the slash itself.
				if (slashPos == std::string::npos)
					slashPos = (unsigned int) -1; 

				GtkTreeIter iter;
				gtk_tree_store_append(_store, &iter, parIter);
				gtk_tree_store_set(_store, &iter, 
						NAME_COLUMN, dirPath.substr(slashPos + 1).c_str(), 
						IMAGE_COLUMN, gtkutil::getLocalPixbuf(imgPath),
						-1);
				GtkTreeIter* dynIter = gtk_tree_iter_copy(&iter); // get a heap-allocated iter

				// Determine if this model has any associated skins, and add them as
				// children
				ModelSkinList skins = GlobalModelSkinCache().getSkinsForModel("models/" + dirPath);
				for (ModelSkinList::iterator i = skins.begin(); i != skins.end(); ++i) {
					GtkTreeIter skIter;
					gtk_tree_store_append(_store, &skIter, &iter);
					gtk_tree_store_set(_store, &skIter, 
							NAME_COLUMN, i->c_str(), 
							IMAGE_COLUMN, gtkutil::getLocalPixbuf("skin16.png"),
							-1);
				}
				
				// Now add a map entry that maps our directory name to the row we just
				// added
				_dirIterMap[dirPath] = dynIter;
				
				// Return our new dynamic iter. 
				return dynIter;

			}				

		}

		// Functor operator
		
		void operator() (const char* file) {

			std::string rawPath(file);			

			// Test the extension. If it is not LWO or ASE (case-insensitive),
			// not interested
			if (!boost::algorithm::iends_with(rawPath, "lwo") 
					&& !boost::algorithm::iends_with(rawPath, "ase")) 
			{
				return;
			}
			else 
			{
				addRecursive(rawPath);
			}
							   
		}

	
	};

}

// Helper function to create the TreeView

GtkWidget* ModelSelector::createTreeView() {

	// Populate the treestore using the VFS callback functor
	
	ModelFileFunctor functor(_treeStore);
	GlobalFileSystem().forEachFile("models/", "*", makeCallback1(functor), 0);

	GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));

	// Single visible column, containing the directory/model name and the icon
	
    GtkTreeViewColumn* col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Value");
    gtk_tree_view_column_set_spacing(col, 3);

	GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(col, pixRenderer, "pixbuf", IMAGE_COLUMN, NULL);

	GtkCellRenderer* rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, rend, FALSE);
	gtk_tree_view_column_set_attributes(col, rend, "text", NAME_COLUMN, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), col);				
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), FALSE);

	// Pack treeview into a scrolled window and return
	
	GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollWin), treeView);
	return scrollWin;
	
}

/* GTK CALLBACKS */

void ModelSelector::callbackHide(GtkWidget* widget, GdkEvent* ev, ModelSelector* self) {
	gtk_main_quit(); // exit recursive main loop
	gtk_widget_hide(self->_widget);
}

}
