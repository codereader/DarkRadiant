#ifndef TEXTCOLUMN_H_
#define TEXTCOLUMN_H_

#include <gtk/gtktreeviewcolumn.h>
#include <gtk/gtkcellrenderertext.h>

namespace gtkutil
{

/** A GtkTreeViewColumn that displays the text contained in the provided
 * column number. The text is interpreted as Pango markup to allow formatting
 * such as bold text.
 */

class TextColumn
{
	// Column widget
	GtkTreeViewColumn* _column;
	
public:

	/** Create a TextColumn which displays the text in the given column.
	 * 
	 * @param title
	 * The title of the column.
	 * 
	 * @param colno
	 * The integer column id to display text from.
	 */
	TextColumn(const std::string& title, gint colno) {
		
		// Create the cell renderer
		GtkCellRenderer* rend = gtk_cell_renderer_text_new();
		
		// Construct the column itself
		_column = gtk_tree_view_column_new_with_attributes(title.c_str(),
														   rend,
														   "markup", colno,
														   NULL);
	}
	
	/** Operator cast to GtkTreeViewColumn*.
	 */
	operator GtkTreeViewColumn* () {
		return _column;	
	}
};

}

#endif /*TEXTCOLUMN_H_*/
