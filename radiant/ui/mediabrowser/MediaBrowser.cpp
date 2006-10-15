#include "MediaBrowser.h"

#include "ishaders.h"
#include "generic/callback.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkscrolledwindow.h>

#include <iostream>
#include <ext/hash_map>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ui
{

// Constructor
MediaBrowser::MediaBrowser()
: _widget(gtk_vbox_new(FALSE, 0)),
  _treeStore(gtk_tree_store_new(1, G_TYPE_STRING))
{
	// Create the treeview
	GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	g_signal_connect(G_OBJECT(view), "expose-event", G_CALLBACK(_onExpose), this);
	
	// Single text column
	GtkCellRenderer* rend = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col = gtk_tree_view_column_new();
	
	gtk_tree_view_column_pack_start(col, rend, FALSE);
	gtk_tree_view_column_set_attributes(col, rend, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	// Pack the treeview into a scrollwindow, frame and then into the vbox
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), view);

	GtkWidget* frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), scroll);
	gtk_box_pack_start(GTK_BOX(_widget), frame, TRUE, TRUE, 0);
}

/* Callback functor for processing shader names */

namespace {
	
	struct ShaderNameFunctor {
		
		typedef const char* first_argument_type;
		
		// TreeStore to populate
		GtkTreeStore* _store;
		
		// Constructor
		ShaderNameFunctor(GtkTreeStore* store)
		: _store(store) {}
		
		// Map between string directory names and their corresponding Iters
		typedef __gnu_cxx::hash_map<std::string, GtkTreeIter*, boost::hash<std::string> > DirIterMap;
		DirIterMap _dirIterMap;

		// Recursive function to add a folder (e.g. "textures/common/something") to the
		// tree, returning the GtkTreeIter* pointing to the newly-added folder. All 
		// parent folders ("textures/common", "textures/") will be added automatically
		// and their iters cached for fast lookup.
		GtkTreeIter* addFolder(const std::string& pathName) {

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
				parIter = addFolder(parentPath);

			// Now add "this directory" as a child, saving the iter in the map
			// and returning it.
			GtkTreeIter iter;
			gtk_tree_store_append(_store, &iter, parIter);
			gtk_tree_store_set(_store, &iter, 0, thisDir.c_str(), -1);
			GtkTreeIter* dynIter = gtk_tree_iter_copy(&iter); // get a heap-allocated iter
			
			// Cache the dynamic iter and return it
			_dirIterMap[pathName] = dynIter;
			return dynIter;
		}
		
		// Functor operator
		
		void operator() (const char* name) {
			std::string rawName(name);
			
			// If the name starts with "textures/", add it to the treestore.
			if (boost::algorithm::istarts_with(rawName, "textures/")) {
				// Separate path into the directory path and texture name
				unsigned int slashPos = rawName.rfind("/");
				const std::string dirPath = rawName.substr(0, slashPos);
				const std::string texName = rawName.substr(slashPos + 1);

				// Recursively add the directory path
				GtkTreeIter* parentIter = addFolder(dirPath);
				
				GtkTreeIter iter;
				gtk_tree_store_append(_store, &iter, parentIter);
				gtk_tree_store_set(_store, &iter, 0, texName.c_str(), -1);
			}
		}
		
	};
	
} // namespace

/* GTK CALLBACKS */

void MediaBrowser::_onExpose(GtkWidget* widget, GdkEventExpose* ev, MediaBrowser* self) {
	// Populate the tree view if it is not already populated
	static bool _isPopulated = false;
	if (!_isPopulated) {
		ShaderNameFunctor functor(self->_treeStore);
		GlobalShaderSystem().foreachShaderName(makeCallback1(functor));

		_isPopulated = true;	
	}
}

}
