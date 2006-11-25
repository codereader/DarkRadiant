#include "TreeModel.h"

namespace gtkutil
{

// Extract a string from a TreeModel
std::string TreeModel::getString(GtkTreeModel* model, GtkTreeIter* iter, gint colNo) {
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

}
