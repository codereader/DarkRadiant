#include "TreeModel.h"

#include "pointer.h"
#include <gtkmm/treeview.h>
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
	// Retrieve the string from the model
	std::string str = getString(model, iter, column);

	// Use a case-insensitive search
	boost::iterator_range<std::string::iterator> range = boost::algorithm::ifind_first(str, key);

	// Returning FALSE means "match".
	return (!range.empty()) ? FALSE: TRUE;
}

bool TreeModel::equalFuncStringContainsmm(const Glib::RefPtr<Gtk::TreeModel>& model, 
										int column, 
										const Glib::ustring& key, 
										const Gtk::TreeModel::iterator& iter)
{
	// Retrieve the string from the model
	std::string str;
	iter->get_value(column, str);

	// Use a case-insensitive search
	boost::iterator_range<std::string::iterator> range = boost::algorithm::ifind_first(str, key);

	// Returning FALSE means "match".
	return (!range.empty()) ? false : true;
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

TreeModel::SelectionFindermm::SelectionFindermm(const std::string& selection, int column) : 
	_selection(selection),
	_needle(0),
	_column(column),
	_searchForInt(false)
{}

TreeModel::SelectionFindermm::SelectionFindermm(int needle, int column) : 
	_selection(""),
	_needle(needle),
	_column(column),
	_searchForInt(true)
{}

const Gtk::TreeModel::iterator TreeModel::SelectionFindermm::getIter() const
{
	return _foundIter;
}

bool TreeModel::SelectionFindermm::forEach(const Gtk::TreeModel::iterator& iter)
{
	// If the visited row matches the string/int to find
	if (_searchForInt)
	{
		int value;
		iter->get_value(_column, value);
		
		if (value == _needle)
		{
			_foundIter = iter;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// Search for string
		Glib::ustring value;
		iter->get_value(_column, value);

		if (value == _selection)
		{
			_foundIter = iter;
			return true;
		}
		else
		{
			return false;
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

bool TreeModel::findAndSelectString(Gtk::TreeView* view, const std::string& needle, int column)
{
	SelectionFindermm finder(needle, column);

	view->get_model()->foreach_iter(sigc::mem_fun(finder, &SelectionFindermm::forEach));

	if (finder.getIter())
	{
		Gtk::TreeModel::Path path(finder.getIter());

		// Expand the treeview to display the target row
		view->expand_to_path(path);
		// Highlight the target row
		view->set_cursor(path);
		// Make the selected row visible 
		view->scroll_to_row(path, 0.3f);

		return true; // found
	}

	return false; // not found
}

bool TreeModel::findAndSelectInteger(Gtk::TreeView* view, int needle, int column)
{
	SelectionFindermm finder(needle, column);

	view->get_model()->foreach_iter(sigc::mem_fun(finder, &SelectionFindermm::forEach));

	if (finder.getIter())
	{
		Gtk::TreeModel::Path path(finder.getIter());

		// Expand the treeview to display the target row
		view->expand_to_path(path);
		// Highlight the target row
		view->set_cursor(path);
		// Make the selected row visible 
		view->scroll_to_row(path, 0.3f);

		return true; // found
	}

	return false; // not found
}

bool TreeModel::findAndSelectString(Gtk::TreeView* view, const std::string& needle, 
									const Gtk::TreeModelColumn<Glib::ustring>& column)
{
	return findAndSelectString(view, needle, column.index());
}

bool TreeModel::findAndSelectInteger(Gtk::TreeView* view, int needle, 
									const Gtk::TreeModelColumn<int>& column)
{
	return findAndSelectInteger(view, needle, column.index());
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

int TreeModel::sortFuncFoldersFirstmm(const Gtk::TreeModel::iterator& a, 
									  const Gtk::TreeModel::iterator& b, 
									  const Gtk::TreeModelColumn<Glib::ustring>& nameColumn,
									  const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	Gtk::TreeModel::Row rowA = *a;
	Gtk::TreeModel::Row rowB = *b;

	// Check if A or B are folders
	bool aIsFolder = rowA[isFolderColumn];
	bool bIsFolder = rowB[isFolderColumn];

	if (aIsFolder)
	{
		// A is a folder, check if B is as well
		if (bIsFolder)
		{
			// A and B are both folders, compare names
			std::string aName = Glib::ustring(rowA[nameColumn]);
			std::string bName = Glib::ustring(rowB[nameColumn]);

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
			std::string aName = Glib::ustring(rowA[nameColumn]);
			std::string bName = Glib::ustring(rowB[nameColumn]);

			// greebo: We're not checking for equality here, names should be unique
			return (aName < bName) ? -1 : 1;
		}
	}
}

void TreeModel::applyFoldersFirstSortFunc(const Glib::RefPtr<Gtk::TreeSortable>& model, 
										  const Gtk::TreeModelColumn<Glib::ustring>& nameColumn,
										  const Gtk::TreeModelColumn<bool>& isFolderColumn)
{
	// Set the sort column ID to nameCol
	model->set_sort_column(nameColumn, Gtk::SORT_ASCENDING);

	// Then apply the custom sort function, bind the folder column to the functor
	model->set_sort_func(nameColumn, sigc::bind(sigc::ptr_fun(sortFuncFoldersFirstmm), nameColumn, isFolderColumn));
}

} // namespace gtkutil
