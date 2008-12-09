#include "MediaBrowser.h"
#include "TextureDirectoryLoader.h"

#include "iuimanager.h"
#include "igroupdialog.h"
#include "ishaders.h"
#include "select.h"
#include "generic/callback.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TreeModel.h"

#include <gtk/gtk.h>

#include <iostream>
#include <map>

#include "selection/algorithm/Shader.h"
#include "selection/shaderclipboard/ShaderClipboard.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ui
{
	
/* CONSTANTS */

namespace {

	const char* FOLDER_ICON = "folder16.png";
	const char* TEXTURE_ICON = "icon_texture.png";
	
	const char* LOAD_TEXTURE_TEXT = "Load in Textures view";
	const char* LOAD_TEXTURE_ICON = "textureLoadInTexWindow16.png";

	const char* APPLY_TEXTURE_TEXT = "Apply to selection";
	const char* APPLY_TEXTURE_ICON = "textureApplyToSelection16.png";

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
  _selection(gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView))),
  _popupMenu(gtkutil::PopupMenu(_treeView)),
  _isPopulated(false)
{
	// Create the treeview
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);
	g_signal_connect(G_OBJECT(_treeView), "expose-event", G_CALLBACK(_onExpose), this);
	
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
	
	// Connect up the selection changed callback
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(_onSelectionChanged), this);
	
	// Construct the popup context menu
	_popupMenu.addItem(
		gtkutil::IconTextMenuItem(
			GlobalRadiant().getLocalPixbuf(LOAD_TEXTURE_ICON), 
			LOAD_TEXTURE_TEXT
		), 
		boost::bind(&MediaBrowser::_onLoadInTexView, this), 
		boost::bind(&MediaBrowser::_testLoadInTexView, this)
	);
	_popupMenu.addItem(
		gtkutil::IconTextMenuItem(
			GlobalRadiant().getLocalPixbuf(APPLY_TEXTURE_ICON), 
			APPLY_TEXTURE_TEXT
		), 
		boost::bind(&MediaBrowser::_onApplyToSel, this), 
		boost::bind(&MediaBrowser::_testSingleTexSel, this)
	);

	// Pack in the TexturePreviewCombo widgets
	gtk_box_pack_end(GTK_BOX(_widget), _preview, FALSE, FALSE, 0);

}

/* Callback functor for processing shader names */

namespace {

struct ShaderNameCompareFunctor : public std::binary_function<std::string, std::string, bool>
{
	bool operator()(const std::string &s1, const std::string &s2) const {
		return boost::algorithm::ilexicographical_compare(s1, s2);
	}
};

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
	typedef std::map<std::string, GtkTreeIter*, ShaderNameCompareFunctor> DirIterMap;
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
		std::size_t slashPos = pathName.rfind("/");
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
						   ICON_COLUMN, GlobalRadiant().getLocalPixbuf(FOLDER_ICON),
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
		if (!boost::algorithm::istarts_with(rawName, "textures/")) {
			rawName = "Other Materials/" + rawName;
		}
		
		{
			// Separate path into the directory path and texture name
			std::size_t slashPos = rawName.rfind("/");
			const std::string dirPath = rawName.substr(0, slashPos);
			const std::string texName = rawName.substr(slashPos + 1);

			// Recursively add the directory path
			GtkTreeIter* parentIter = addFolder(dirPath);
			
			GtkTreeIter iter;
			gtk_tree_store_append(_store, &iter, parentIter);
			gtk_tree_store_set(_store, &iter, 
							   DISPLAYNAME_COLUMN, texName.c_str(), 
							   FULLNAME_COLUMN, name,
							   ICON_COLUMN, GlobalRadiant().getLocalPixbuf(TEXTURE_ICON),
							   DIR_FLAG_COLUMN, FALSE,
							   -1);
		}
	}
	
};
	
} // namespace

/* Tree query functions */

bool MediaBrowser::isDirectorySelected() {
	// Get the selected value
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(_selection, NULL, &iter)) {
		GValue dirFlagVal;
		memset(&dirFlagVal, 0, sizeof(GValue));
		gtk_tree_model_get_value(GTK_TREE_MODEL(_treeStore), &iter, DIR_FLAG_COLUMN, &dirFlagVal);
		// Return boolean value
		return g_value_get_boolean(&dirFlagVal) ? true : false;
	}
	else {
		// Error condition if there is no selection
		return false;
	}
}

std::string MediaBrowser::getSelectedName() {
	// Get the selected value
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(_selection, NULL, &iter)) {
		GValue nameVal;
		memset(&nameVal, 0, sizeof(GValue));
		gtk_tree_model_get_value(GTK_TREE_MODEL(_treeStore), &iter, FULLNAME_COLUMN, &nameVal);
		// Return boolean value
		return g_value_get_string(&nameVal);
	}
	else {
		// Error condition if there is no selection
		return "";
	}
}

/** Return the singleton instance.
 */
MediaBrowser& MediaBrowser::getInstance() {
	static MediaBrowser _instance;
	return _instance;
}

// Set the selection in the treeview
void MediaBrowser::setSelection(const std::string& selection) {
	if (!_isPopulated) {
		populate();
	}
	
	// If the selection string is empty, collapse the treeview and return with
	// no selection
	if (selection.empty()) {
		gtk_tree_view_collapse_all(GTK_TREE_VIEW(_treeView));
		return;
	}
	// Use the local SelectionFinder class to walk the TreeModel
	gtkutil::TreeModel::SelectionFinder finder(selection, FULLNAME_COLUMN);
	
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(_treeView));
	gtk_tree_model_foreach(model, gtkutil::TreeModel::SelectionFinder::forEach, &finder);
		
	// Get the found TreePath (may be NULL)
	GtkTreePath* path = finder.getPath();
	if (path) {
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(_treeView), path);
		// Highlight the target row
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(_treeView), path, NULL, false);
		// Make the selected row visible 
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(_treeView), path, NULL, true, 0.3f, 0.0f);
	}
}

void MediaBrowser::reloadMedia() {
	// Remove all items and clear the "isPopulated" flag
	gtk_tree_store_clear(_treeStore);
	_isPopulated = false;
	
	// Trigger an "expose" event
	gtk_widget_queue_draw(_widget);
} 

void MediaBrowser::populate() {
	// Set the flag to true to avoid double-entering this function
	_isPopulated = true;

	ShaderNameFunctor functor(_treeStore);
	
	// greebo: Add the textures folder first, so that it gets displayed before
	// the "Other Materials" folder.
	functor.addFolder("textures");
	GlobalShaderSystem().foreachShaderName(makeCallback1(functor));	
}

/* gtkutil::PopupMenu callbacks */

void MediaBrowser::_onLoadInTexView() {
	// Use a TextureDirectoryLoader functor to search the directory. This 
	// may throw an exception if cancelled by user.
	TextureDirectoryLoader loader(getSelectedName());
	try {
		GlobalShaderSystem().foreachShaderName(makeCallback1(loader));
	}
	catch (gtkutil::ModalProgressDialog::OperationAbortedException e) {
		// Ignore the error and return from the function normally	
	}
}

bool MediaBrowser::_testLoadInTexView() {
	// "Load in textures view" requires a directory selection
	if (isDirectorySelected())
		return true;
	else
		return false;
}

void MediaBrowser::_onApplyToSel() {
	// Pass shader name to the selection system
	selection::algorithm::applyShaderToSelection(getSelectedName());
}

// Check if a single non-directory texture is selected (used by multiple menu
// options).
bool MediaBrowser::_testSingleTexSel() {
	if (!isDirectorySelected() && getSelectedName() != "")
		return true;
	else
		return false;
}

/* GTK CALLBACKS */

gboolean MediaBrowser::_onExpose(GtkWidget* widget, GdkEventExpose* ev, MediaBrowser* self) {
	// Populate the tree view if it is not already populated
	if (!self->_isPopulated) {
		self->populate();
	}
	return FALSE; // progapagate event
}

void MediaBrowser::_onSelectionChanged(GtkWidget* widget, MediaBrowser* self) {
	// Update the preview if a texture is selected
	if (!self->isDirectorySelected()) {
		self->_preview.setTexture(self->getSelectedName());
		GlobalShaderClipboard().setSource(self->getSelectedName());
	}
	else {
		// Nothing selected, clear the clipboard
		GlobalShaderClipboard().clear();
	}
}

void MediaBrowser::toggle() {
	GlobalGroupDialog().togglePage("mediabrowser");
}

}
