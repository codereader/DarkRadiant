#ifndef TREEMODEL_H_
#define TREEMODEL_H_

#include <string>

#include <gtk/gtktreemodel.h>
#include <gtk/gtktreeselection.h>

namespace gtkutil
{

/** Utility class for operation on GtkTreeModels. This class provides
 * methods to retrieve strings and other values from a tree model without
 * having to manually use GValues etc.
 */

class TreeModel
{
public:

	/** Extract a string from the given row of the provided TreeModel.
	 * 
	 * @param model
	 * The TreeModel* to examine.
	 * 
	 * @param iter
	 * A GtkTreeIter pointing to the row to look up.
	 * 
	 * @param colNo
	 * The column number to look up.
	 */
	static std::string getString(GtkTreeModel* model, 
								 GtkTreeIter* iter, 
								 gint colNo);

	/** Extract a boolean from the given row of the provided TreeModel.
	 * 
	 * @param model
	 * The TreeModel* to examine.
	 * 
	 * @param iter
	 * A GtkTreeIter pointing to the row to look up.
	 * 
	 * @param colNo
	 * The column number to look up.
	 */
	static bool getBoolean(GtkTreeModel* model, GtkTreeIter* iter, gint colNo);

	/**
	 * Extract the selected string from the given column in the TreeModel. The
	 * selection object will be queried for a selection, and the string
	 * returned if present, otherwise an empty string will be returned.
	 * 
	 * @param selection
	 * GtkTreeSelection object to be tested for a selection.
	 * 
	 * @param colNo
	 * The column number to extract data from if the selection is valid.
	 */
	static std::string getSelectedString(GtkTreeSelection* selection,
										 gint colNo);
};

}

#endif /*TREEMODEL_H_*/
