#include "MediaBrowser.h"
#include "TextureDirectoryLoader.h"

#include "imainframe.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ipreferencesystem.h"
#include "ishaders.h"
#include "iregistry.h"
#include "select.h"
#include "generic/callback.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"

#include <gtk/gtk.h>

#include <iostream>
#include <map>

#include "selection/algorithm/Shader.h"
#include "selection/shaderclipboard/ShaderClipboard.h"

#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/common/ShaderDefinitionView.h"

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

	const char* SHOW_SHADER_DEF_TEXT = "Show Shader Definition";
	const char* SHOW_SHADER_DEF_ICON = "icon_script.png";

	// TreeStore columns
	enum {
		DISPLAYNAME_COLUMN,
		FULLNAME_COLUMN,
		ICON_COLUMN,
		DIR_FLAG_COLUMN,
		IS_OTHER_MATERIALS_FOLDER_COLUMN,
		N_COLUMNS
	};
	
	const std::string RKEY_MEDIA_BROWSER_PRELOAD = "user/ui/mediaBrowser/preLoadMediaTree";
	const std::string OTHER_MATERIALS_FOLDER = "Other Materials";
}

// Constructor
MediaBrowser::MediaBrowser()
: _widget(gtk_vbox_new(FALSE, 0)),
  _treeStore(gtk_tree_store_new(N_COLUMNS, 
  								G_TYPE_STRING, 
  								G_TYPE_STRING,
  								GDK_TYPE_PIXBUF,
  								G_TYPE_BOOLEAN,
								G_TYPE_BOOLEAN)),
  _treeView(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore))),
  _selection(gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView))),
  _popupMenu(gtkutil::PopupMenu(_treeView)),
  _isPopulated(false)
{
	// Create the treeview
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);
	g_signal_connect(G_OBJECT(_treeView), "expose-event", G_CALLBACK(_onExpose), this);
	
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(_treeView), 
		gtkutil::IconTextColumn("Shader", DISPLAYNAME_COLUMN, ICON_COLUMN)
	);

	// Set the tree store to sort on this column
    gtk_tree_sortable_set_sort_column_id(
        GTK_TREE_SORTABLE(_treeStore),
        DISPLAYNAME_COLUMN,
        GTK_SORT_ASCENDING
    );

	// Set the custom sort function
	gtk_tree_sortable_set_sort_func(
		GTK_TREE_SORTABLE(_treeStore),
		DISPLAYNAME_COLUMN,	// sort column
		treeViewSortFunc,	// function
		this,				// userdata
		NULL				// no destroy notify
	);

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(_treeView), gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);
	
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
			GlobalUIManager().getLocalPixbuf(LOAD_TEXTURE_ICON), 
			LOAD_TEXTURE_TEXT
		), 
		boost::bind(&MediaBrowser::_onLoadInTexView, this), 
		boost::bind(&MediaBrowser::_testLoadInTexView, this)
	);
	_popupMenu.addItem(
		gtkutil::IconTextMenuItem(
			GlobalUIManager().getLocalPixbuf(APPLY_TEXTURE_ICON), 
			APPLY_TEXTURE_TEXT
		), 
		boost::bind(&MediaBrowser::_onApplyToSel, this), 
		boost::bind(&MediaBrowser::_testSingleTexSel, this)
	);
	_popupMenu.addItem(
		gtkutil::IconTextMenuItem(
			GlobalUIManager().getLocalPixbuf(SHOW_SHADER_DEF_ICON), 
			SHOW_SHADER_DEF_TEXT
		), 
		boost::bind(&MediaBrowser::_onShowShaderDefinition, this), 
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
	GtkTreeIter* addFolder(const std::string& pathName, bool isOtherMaterials = false) {

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
						   ICON_COLUMN, GlobalUIManager().getLocalPixbuf(FOLDER_ICON),
						   DIR_FLAG_COLUMN, TRUE,
						   IS_OTHER_MATERIALS_FOLDER_COLUMN, isOtherMaterials ? TRUE : FALSE,
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
			rawName = OTHER_MATERIALS_FOLDER + "/" + rawName;
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
							   ICON_COLUMN, GlobalUIManager().getLocalPixbuf(TEXTURE_ICON),
							   DIR_FLAG_COLUMN, FALSE,
							   IS_OTHER_MATERIALS_FOLDER_COLUMN, FALSE,
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
		return gtkutil::TreeModel::getBoolean(GTK_TREE_MODEL(_treeStore), &iter, DIR_FLAG_COLUMN);
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
		return gtkutil::TreeModel::getString(GTK_TREE_MODEL(_treeStore), &iter, FULLNAME_COLUMN);
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

void MediaBrowser::init() {
	// Check for pre-loading the textures
	if (GlobalRegistry().get(RKEY_MEDIA_BROWSER_PRELOAD) == "1") {
		getInstance().populate();
	}
}

void MediaBrowser::populate() {
	// Set the flag to true to avoid double-entering this function
	_isPopulated = true;

	ShaderNameFunctor functor(_treeStore);
	
	// greebo: Add the Other Materials folder and pass TRUE to indicate this is a special one
	functor.addFolder(OTHER_MATERIALS_FOLDER, true);
	GlobalMaterialManager().foreachShaderName(makeCallback1(functor));	
}

/* gtkutil::PopupMenu callbacks */

void MediaBrowser::_onLoadInTexView() {
	// Use a TextureDirectoryLoader functor to search the directory. This 
	// may throw an exception if cancelled by user.
	TextureDirectoryLoader loader(getSelectedName());
	try {
		GlobalMaterialManager().foreachShaderName(makeCallback1(loader));
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

void MediaBrowser::_onShowShaderDefinition()
{
	std::string shaderName = getSelectedName();

	// Construct a shader view and pass the shader name
	ShaderDefinitionView view;
	view.setShader(shaderName);

	GtkWidget* dialog = gtk_dialog_new_with_buttons("View Shader Definition", GlobalMainFrame().getTopLevelWindow(),
                                         GTK_DIALOG_DESTROY_WITH_PARENT, 
                                         GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                         NULL);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 12);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), view.getWidget());

	GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());
	gtk_window_set_default_size(
		GTK_WINDOW(dialog), gint(rect.width/2), gint(2*rect.height/3)
	);

	gtk_widget_show_all(dialog);

	// Show and block
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
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

gint MediaBrowser::treeViewSortFunc(GtkTreeModel *model, 
									GtkTreeIter *a, 
									GtkTreeIter *b, 
									gpointer user_data)
{
	// Special treatment for "Other Materials" folder, which always comes last
	if (gtkutil::TreeModel::getBoolean(model, a, IS_OTHER_MATERIALS_FOLDER_COLUMN)) {
		return 1;
	}

	if (gtkutil::TreeModel::getBoolean(model, b, IS_OTHER_MATERIALS_FOLDER_COLUMN)) {
		return -1;
	}

	// Check if A or B are folders
	bool aIsFolder = gtkutil::TreeModel::getBoolean(model, a, DIR_FLAG_COLUMN);
	bool bIsFolder = gtkutil::TreeModel::getBoolean(model, b, DIR_FLAG_COLUMN);

	if (aIsFolder) {
		// A is a folder, check if B is as well
		if (bIsFolder) {
			// A and B are both folders, compare names
			std::string aName = gtkutil::TreeModel::getString(model, a, DISPLAYNAME_COLUMN);
			std::string bName = gtkutil::TreeModel::getString(model, b, DISPLAYNAME_COLUMN);

			// greebo: We're not checking for equality here, shader names are unique
			return (aName < bName) ? -1 : 1;
		}
		else {
			// A is a folder, B is not, A sorts before
			return -1;
		}
	}
	else {
		// A is not a folder, check if B is one
		if (bIsFolder) {
			// A is not a folder, B is, so B sorts before A
			return 1;
		}
		else {
			// Neither A nor B are folders, compare names
			std::string aName = gtkutil::TreeModel::getString(model, a, DISPLAYNAME_COLUMN);
			std::string bName = gtkutil::TreeModel::getString(model, b, DISPLAYNAME_COLUMN);

			// greebo: We're not checking for equality here, shader names are unique
			return (aName < bName) ? -1 : 1;
		}
	}
}

void MediaBrowser::toggle(const cmd::ArgumentList& args) {
	GlobalGroupDialog().togglePage("mediabrowser");
}

void MediaBrowser::registerPreferences() {
	// Add a page to the given group
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Media Browser");
	
	page->appendCheckBox("", "Load media tree at startup", RKEY_MEDIA_BROWSER_PRELOAD);
}

} // namespace ui
