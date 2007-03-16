#ifndef ICONTEXTCOLUMN_H_
#define ICONTEXTCOLUMN_H_

#include <gtk/gtktreeviewcolumn.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrendererpixbuf.h>

namespace gtkutil
{
	
/**
 * A TreeViewColumn which contains an icon and a text value, the contents of 
 * which are both retrieved from specified columns in the tree model.
 */
class IconTextColumn
{
	// Tree column widget
	GtkTreeViewColumn* _column;
	
public:

	/**
	 * Construct an IconTextColumn with values retrieved from the specified
	 * columns.
	 * 
	 * @param title
	 * The text title of the column.
	 * 
	 * @param textCol
	 * TreeModel column containing text to display.
	 * 
	 * @param iconCol
	 * TreeModel column containing the icon.
	 * 
	 * @param useMarkup
	 * Whether Pango markup should be used to set the text for the column.
	 */
	IconTextColumn(const std::string& title, 
				   gint textCol, 
				   gint iconCol, 
				   bool useMarkup = false) 
	{
		// Create the TreeViewColumn
		_column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(_column, title.c_str());
		gtk_tree_view_column_set_spacing(_column, 3);
		
		// Add the icon
		GtkCellRenderer* pixRend = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start(_column, pixRend, FALSE);
		gtk_tree_view_column_set_attributes(_column, pixRend, 
											"pixbuf", iconCol,
											NULL);

		// Add the text
		GtkCellRenderer* textRend = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(_column, textRend, FALSE);
		gtk_tree_view_column_set_attributes(
			_column, textRend, 
			useMarkup ? "markup" : "text", textCol,
			NULL);
	}
	
	/**
	 * Operator cast to GtkTreeViewColumn* for packing into a tree view.
	 */
	operator GtkTreeViewColumn* () {
		return _column;
	}
};

}

#endif /*ICONTEXTCOLUMN_H_*/
