#include "TreeModel.h"

#include "pointer.h"
#include <boost/algorithm/string/find.hpp>

namespace gtkutil
{

// Extract a string from a TreeModel
std::string TreeModel::getString(GtkTreeModel* model, 
								 GtkTreeIter* iter, 
								 gint colNo) 
{
	// Get a GValue
	GValue val = {0, {{0}}};
	gtk_tree_model_get_value(model, iter, colNo, &val);

	// Create and return the string, and free the GValue
	const gchar* c = g_value_get_string(&val);

	// greebo: g_value_get_string can return NULL, catch this
	std::string retVal = (c != NULL) ? c : "";

	g_value_unset(&val);
	return retVal;
}

// Extract a boolean from a TreeModel
bool TreeModel::getBoolean(GtkTreeModel* model, GtkTreeIter* iter, gint colNo) {
	// Get a GValue
	GValue val = {0, {{0}}};
	gtk_tree_model_get_value(model, iter, colNo, &val);

	// Create and return the string, and free the GValue
	bool retVal = g_value_get_boolean(&val) ? true: false;
	g_value_unset(&val);
	return retVal;
}

// Extract a boolean from a TreeModel
int TreeModel::getInt(GtkTreeModel* model, GtkTreeIter* iter, gint colNo) {
	// Get a GValue
	GValue val = {0, {{0}}};
	gtk_tree_model_get_value(model, iter, colNo, &val);

	// Create and return the string, and free the GValue
	int retVal = g_value_get_int(&val);
	g_value_unset(&val);
	return retVal;
}

// Extract a boolean from a TreeModel
gpointer TreeModel::getPointer(GtkTreeModel* model, GtkTreeIter* iter, gint colNo) {
	// Get a GValue
	GValue val = {0, {{0}}};
	gtk_tree_model_get_value(model, iter, colNo, &val);

	// Create and return the string, and free the GValue
	gpointer retVal = g_value_get_pointer(&val);
	g_value_unset(&val);
	return retVal;
}

// Extract a selected string
std::string TreeModel::getSelectedString(GtkTreeSelection* sel, gint colNo)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	
	// Get the selected value by querying the selection object
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
		return getString(model,	&iter, colNo);
	}
	else {
		// Nothing selected, return empty string
		return "";
	}
}

bool TreeModel::getSelectedBoolean(GtkTreeSelection* selection, gint colNo)
{
	GtkTreeIter iter;
	GtkTreeModel* model;
	
	// Get the selected value by querying the selection object
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		return getBoolean(model, &iter, colNo);
	}
	else {
		// Nothing selected, return false
		return false;
	}
}

gboolean TreeModel::equalFuncStringContains(GtkTreeModel* model, 
										gint column,
										const gchar* key,
										GtkTreeIter* iter,
										gpointer search_data)
{
	// Retrieve the eclass string from the model
	std::string str = getString(model, iter, column);

	// Use a case-insensitive search
	boost::iterator_range<std::string::iterator> range = boost::algorithm::ifind_first(str, key);

	// Returning FALSE means "match".
	return (!range.empty()) ? FALSE: TRUE;
}

TreeModel::SelectionFinder::SelectionFinder(const std::string& selection, int column) : 
	_selection(selection),
	_needle(0),
	_path(NULL),
	_model(NULL),
	_column(column),
	_searchForInt(false)
{}

TreeModel::SelectionFinder::SelectionFinder(int needle, int column) : 
	_selection(""),
	_needle(needle),
	_path(NULL),
	_model(NULL),
	_column(column),
	_searchForInt(true)
{}

TreeModel::SelectionFinder::~SelectionFinder()
{
	if (_path != NULL)
	{
		gtk_tree_path_free(_path);
		_path = NULL;
	}
}

GtkTreePath* TreeModel::SelectionFinder::getPath() {
	return _path;
}

GtkTreeIter TreeModel::SelectionFinder::getIter() {
	// The TreeIter structure to be returned
	GtkTreeIter iter;
	
	// Convert the retrieved path into a GtkTreeIter
	if (_path != NULL) {
		gtk_tree_model_get_iter_from_string(
			GTK_TREE_MODEL(_model),
			&iter,
			gtk_tree_path_to_string(_path)
		);
	}
	
	return iter;
}

// Static callback for GTK
gboolean TreeModel::SelectionFinder::forEach(
	GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer vpSelf) 
{
	// Get the self instance from the void pointer
	TreeModel::SelectionFinder* self = 
		reinterpret_cast<TreeModel::SelectionFinder*>(vpSelf);

	// Store the TreeModel pointer for later reference
	self->_model = model;
		
	// If the visited row matches the texture to find, set the _path
	// variable and finish, otherwise continue to search
	
	if (self->_searchForInt) {
		if (TreeModel::getInt(model, iter, self->_column) == self->_needle) {
			self->_path = gtk_tree_path_copy(path);
			return TRUE; // finish the walk
		}
		else {
			return FALSE;
		}
	}
	else {
		// Search for string
		if (TreeModel::getString(model, iter, self->_column) == self->_selection) {
			self->_path = gtk_tree_path_copy(path);
			return TRUE; // finish the walk
		}
		else {
			return FALSE;
		}
	}
}

gint TreeModel::sortFuncFoldersFirst(GtkTreeModel* model, 
									 GtkTreeIter *a, 
									 GtkTreeIter *b, 
									 gpointer isFolderColumn)
{
	gint isFolderCol = gpointer_to_int(isFolderColumn);

	GtkSortType sort;
	gint nameCol = 0;
	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &nameCol, &sort);

	// Check if A or B are folders
	bool aIsFolder = getBoolean(model, a, isFolderCol);
	bool bIsFolder = getBoolean(model, b, isFolderCol);

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders, compare names
			std::string aName = getString(model, a, nameCol);
			std::string bName = getString(model, b, nameCol);

			// greebo: We're not checking for equality here, names should be unique
			return (aName < bName) ? -1 : 1;
		}
		else
		{
			// A is a folder, B is not, A sorts before
			return -1;
		}
	}
	else
	{
		// A is not a folder, check if B is one
		if (bIsFolder)
		{
			// A is not a folder, B is, so B sorts before A
			return 1;
		}
		else
		{
			// Neither A nor B are folders, compare names
			std::string aName = getString(model, a, nameCol);
			std::string bName = getString(model, b, nameCol);

			// greebo: We're not checking for equality here, names should be unique
			return (aName < bName) ? -1 : 1;
		}
	}
}

bool TreeModel::findAndSelectString(GtkTreeView* view, const std::string& needle, int column)
{
	// Find the selection string in the model
	SelectionFinder finder(needle, column);
	
	GtkTreeModel* model = gtk_tree_view_get_model(view);
	gtk_tree_model_foreach(model, SelectionFinder::forEach, &finder);
	
	// Get the found TreePath (may be NULL)
	GtkTreePath* path = finder.getPath();

	if (path != NULL)
	{
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(view, path);
		// Highlight the target row
		gtk_tree_view_set_cursor(view, path, NULL, false);
		// Make the selected row visible 
		gtk_tree_view_scroll_to_cell(view, path, NULL, true, 0.3f, 0.0f);

		return true;
	}

	return false; // not found
}

bool TreeModel::findAndSelectInteger(GtkTreeView* view, int needle, int column)
{
	// Find the selection string in the model
	SelectionFinder finder(needle, column);
	
	GtkTreeModel* model = gtk_tree_view_get_model(view);
	gtk_tree_model_foreach(model, SelectionFinder::forEach, &finder);
	
	// Get the found TreePath (may be NULL)
	GtkTreePath* path = finder.getPath();

	if (path != NULL)
	{
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(view, path);
		// Highlight the target row
		gtk_tree_view_set_cursor(view, path, NULL, false);
		// Make the selected row visible 
		gtk_tree_view_scroll_to_cell(view, path, NULL, true, 0.3f, 0.0f);

		return true;
	}

	return false; // not found
}

void TreeModel::applyFoldersFirstSortFunc(GtkTreeModel* model, 
										  gint nameCol, gint isFolderColumn)
{
	// Set the sort column ID to nameCol
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(model),
		nameCol,
		GTK_SORT_ASCENDING
	);

	// Then apply the custom sort functin
	gtk_tree_sortable_set_sort_func(
		GTK_TREE_SORTABLE(model),
		nameCol,							// sort column
		sortFuncFoldersFirst,				// function
		gint_to_pointer(isFolderColumn),	// userdata: pointer-encoded "isFolderColumn"
		NULL								// no destroy notify
	);
}

} // namespace gtkutil
