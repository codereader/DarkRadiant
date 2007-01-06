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
	bool retVal = g_value_get_boolean(&val);
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

}
