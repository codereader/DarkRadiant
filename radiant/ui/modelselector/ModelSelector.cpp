#include "ModelSelector.h"

#include "mainframe.h"

#include "ifilesystem.h"

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
	
}

// Constructor.

ModelSelector::ModelSelector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _treeStore(gtk_tree_store_new(1, G_TYPE_STRING))
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
		
		// Utility function to obtain the GtkTreeIter* pointing to the given model
		// directory. If there is no iter in the DirIterMap, call recursively on the
		// directory's own parent, and add THIS directory as a child.
		
		GtkTreeIter* findParentIter(const std::string& dirPath) {
			std::cout << "findParentIter: finding iter for " << dirPath << std::endl;
			
			// Bottom-out condition: if we have an empty string, no further parent
			// lookups are necessary. Return NULL, which is used by tree_store_append
			// to signify an addition at the top-level.

			if (dirPath == "") {
				std::cout << "  empty string, returning NULL" << std::endl;
				return NULL;
			}
			
			// Otherwise, we first try to lookup the directory name in the map. Return it
			// if it exists, otherwise recursively obtain the parent of this directory name,
			// and add this directory as a child in the tree model. We also add this
			// directory to the map for future lookups.
			
			DirIterMap::iterator iTemp = _dirIterMap.find(dirPath);
			if (iTemp != _dirIterMap.end()) { // found in map
				std::cout << "  FOUND in map" << std::endl;
				return iTemp->second;
			}
			else { 
				std::cout << "  Not found, recursively searching for parents" << std::endl;
				
				// Perform the search for final "/" which identifies the parent
				// of this directory, and call recursively.
				int slashPos = dirPath.rfind("/");
				GtkTreeIter* parIter = findParentIter(slashPos != std::string::npos
													  ? dirPath.substr(0, slashPos)
													  : "");

				// Add this directory to the treemodel
				std::cout << "  Adding " << dirPath << " to treemodel" << std::endl;
				
				// Now add a map entry that maps our directory name to the row we just
				// added
				std::cout << "  Adding " << dirPath << " to iter map" << std::endl;
				_dirIterMap[dirPath] = NULL;
				
				// Return our new iter
				return NULL;

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

			// Get the position of the final "/", everything before this is the
			// containing directory.
			int slashPos = rawPath.rfind("/");
			
			// Now call the recursive findParentIter function, to obtain the GtkTreeIter
			// that corresponds to this models directory.
			GtkTreeIter* parent = findParentIter(rawPath.substr(0, slashPos));

			// Append the model pathname
			GtkTreeIter iter;
			gtk_tree_store_append(_store, &iter, NULL);
			gtk_tree_store_set(_store, &iter, 
							   0, file,
							   -1);
							   
		}

	
	};

}

// Helper function to create the TreeView

GtkWidget* ModelSelector::createTreeView() {

	// Populate the treestore using the VFS callback functor
	
	ModelFileFunctor functor(_treeStore);
	GlobalFileSystem().forEachFile("models/", "*", makeCallback1(functor), 0);

	GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));

	// Single column, containing model pathname
	
	GtkCellRenderer* rend = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col = 
		gtk_tree_view_column_new_with_attributes("Model path", 
									      		 rend,
									      		 "text", 0,
									      		 NULL);
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
