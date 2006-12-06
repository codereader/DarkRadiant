#ifndef TREEMODEL_H_
#define TREEMODEL_H_

#include <string>

#include <gtk/gtktreemodel.h>

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
};

}

#endif /*TREEMODEL_H_*/
