#include "MediaBrowser.h"
#include "TextureDirectoryLoader.h"

#include "ishaders.h"
#include "texwindow.h"
#include "generic/callback.h"
#include "gtkutil/image.h"
#include "gtkutil/TextMenuItem.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtktreeselection.h>

#include <iostream>
#include <ext/hash_map>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ui
{
	
/* CONSTANTS */

namespace {

	const char* FOLDER_ICON = "folder16.png";
	const char* TEXTURE_ICON = "icon_texture.png";
	
	const char* LOAD_TEXTURE_TEXT = "Load in Textures view";

	// TreeStore columns
	enum {
		DISPLAYNAME_COLUMN,
		FULLNAME_COLUMN,
		ICON_COLUMN,
		DIR_FLAG_COLUMN,
		N_COLUMNS
	};
	
}

// Constructor
MediaBrowser::MediaBrowser()
: _widget(gtk_vbox_new(FALSE, 0)),
  _treeStore(gtk_tree_store_new(N_COLUMNS, 
  								G_TYPE_STRING, 
  								G_TYPE_STRING,
  								GDK_TYPE_PIXBUF,
  								G_TYPE_BOOLEAN)),
  _treeView(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore))),
  _popupMenu(gtk_menu_new())
{
	// Create the treeview
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);
	g_signal_connect(G_OBJECT(_treeView), "expose-event", G_CALLBACK(_onExpose), this);
	g_signal_connect(G_OBJECT(_treeView), "button-release-event", G_CALLBACK(_onRightClick), this);
	
	// Single text column with packed icon
	GtkTreeViewColumn* col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_spacing(col, 3);
	
	GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(col, pixRenderer, "pixbuf", ICON_COLUMN, NULL);

	GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, textRenderer, FALSE);
	gtk_tree_view_column_set_attributes(col, textRenderer, "text", DISPLAYNAME_COLUMN, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), col);
	
	// Pack the treeview into a scrollwindow, frame and then into the vbox
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), _treeView);

	GtkWidget* frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), scroll);
	gtk_box_pack_start(GTK_BOX(_widget), frame, TRUE, TRUE, 0);
	
	// Construct the popup context menu
	GtkWidget* loadDirectory = gtkutil::TextMenuItem(LOAD_TEXTURE_TEXT);
	
	g_signal_connect(G_OBJECT(loadDirectory), "activate", G_CALLBACK(_onActivateLoadContained), this);

	gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), loadDirectory);
	
	gtk_widget_show_all(_popupMenu);
	
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
		
		// Destructor. Free all the heap-allocated GtkTreeIters in the
		// map
		~ShaderNameFunctor() {
			for (DirIterMap::iterator i = _dirIterMap.begin();
					i != _dirIterMap.end();
						++i) 
			{
				gtk_tree_iter_free(i->second);
			}
		}
		
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
			gtk_tree_store_set(_store, &iter, 
							   DISPLAYNAME_COLUMN, thisDir.c_str(), 
							   FULLNAME_COLUMN, pathName.c_str(),
							   ICON_COLUMN, gtkutil::getLocalPixbuf(FOLDER_ICON),
							   DIR_FLAG_COLUMN, TRUE,
							   -1);
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
				gtk_tree_store_set(_store, &iter, 
								   DISPLAYNAME_COLUMN, texName.c_str(), 
								   FULLNAME_COLUMN, name,
								   ICON_COLUMN, gtkutil::getLocalPixbuf(TEXTURE_ICON),
								   DIR_FLAG_COLUMN, FALSE,
								   -1);
			}
		}
		
	};
	
} // namespace

/* GTK CALLBACKS */

gboolean MediaBrowser::_onExpose(GtkWidget* widget, GdkEventExpose* ev, MediaBrowser* self) {
	// Populate the tree view if it is not already populated
	static bool _isPopulated = false;
	if (!_isPopulated) {
		ShaderNameFunctor functor(self->_treeStore);
		GlobalShaderSystem().foreachShaderName(makeCallback1(functor));

		_isPopulated = true;	
	}
	return FALSE; // progapagate event
}

bool MediaBrowser::_onRightClick(GtkWidget* widget, GdkEventButton* ev, MediaBrowser* self) {
	// Popup on right-click events only
	if (ev->button == 3) {
		gtk_menu_popup(GTK_MENU(self->_popupMenu), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
	}
	return FALSE;
}

void MediaBrowser::_onActivateLoadContained(GtkMenuItem* item, MediaBrowser* self) {
	// Get the current selection to load
	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->_treeView));
	GtkTreeIter iter;
	GtkTreeModel* model;

	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		// Determine whether this is a directory or an individual texture
		GValue dirFlagVal = {0, 0};
		gtk_tree_model_get_value(model, &iter, DIR_FLAG_COLUMN, &dirFlagVal);
		
		if (g_value_get_boolean(&dirFlagVal)) {
			// Directory
			GValue val = {0, 0};
			gtk_tree_model_get_value(model, &iter, FULLNAME_COLUMN, &val);
			std::string dir = g_value_get_string(&val);
			// Use a TextureDirectoryLoader functor to search the directory
			TextureDirectoryLoader loader(dir);
			GlobalShaderSystem().foreachShaderName(makeCallback1(loader));
		}
		else {
			// Individual texture
			GValue val = {0, 0};
			gtk_tree_model_get_value(model, &iter, FULLNAME_COLUMN, &val);
			// Load the shader by name and release it, to force a load
			IShader* ref = GlobalShaderSystem().getShaderForName(g_value_get_string(&val));
			ref->DecRef();
		}
	}
}

}
