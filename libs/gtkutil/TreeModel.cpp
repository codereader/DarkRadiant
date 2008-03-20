#include "TreeModel.h"

namespace gtkutil
{

// Extract a string from a TreeModel
std::string TreeModel::getString(GtkTreeModel* model, 
								 GtkTreeIter* iter, 
								 gint colNo) 
{
	// Get a GValue
	GValue val = {0, 0};
	gtk_tree_model_get_value(model, iter, colNo, &val);

	// Create and return the string, and free the GValue
	std::string retVal = g_value_get_string(&val);
	g_value_unset(&val);
	return retVal;
}

// Extract a boolean from a TreeModel
bool TreeModel::getBoolean(GtkTreeModel* model, GtkTreeIter* iter, gint colNo) {
	// Get a GValue
	GValue val = {0, 0};
	gtk_tree_model_get_value(model, iter, colNo, &val);

	// Create and return the string, and free the GValue
	bool retVal = g_value_get_boolean(&val) ? true: false;
	g_value_unset(&val);
	return retVal;
}

// Extract a boolean from a TreeModel
int TreeModel::getInt(GtkTreeModel* model, GtkTreeIter* iter, gint colNo) {
	// Get a GValue
	GValue val = {0, 0};
	gtk_tree_model_get_value(model, iter, colNo, &val);

	// Create and return the string, and free the GValue
	int retVal = g_value_get_int(&val);
	g_value_unset(&val);
	return retVal;
}

// Extract a boolean from a TreeModel
gpointer TreeModel::getPointer(GtkTreeModel* model, GtkTreeIter* iter, gint colNo) {
	// Get a GValue
	GValue val = {0, 0};
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

}
