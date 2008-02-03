#ifndef TWOCOLUMNTEXTCOMBO_H_
#define TWOCOLUMNTEXTCOMBO_H_

#include <gtk/gtkliststore.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcelllayout.h>

namespace objectives
{

namespace util
{

/**
 * Helper class to create a GtkComboBox containing two text columns.
 * 
 * This class provides a convenient mechanism to create a GtkComboBox backed
 * by a GtkTreeModel containing two text columns. The first text column (column
 * 0) contains a text string which will be displayed in the GtkComboBox itself,
 * whereas the second (column 1) contains a string which will not be displayed
 * and is intended for use as a code-level identifier which is not visible to
 * the user.
 */
class TwoColumnTextCombo
{
	// Combo widget
	GtkWidget* _combo;
	
public:
	
	/**
	 * Construct a TwoColumnTextCombo.
	 */
	TwoColumnTextCombo()
	: _combo()
	{
		// List store and combo box
		GtkListStore* ls = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING); 
		_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ls));
		
		// Add a text cell renderer for column 0
		GtkCellRenderer* rend = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_combo), rend, FALSE);
		gtk_cell_layout_set_attributes(
			GTK_CELL_LAYOUT(_combo), rend, "text", 0, NULL 
		);
	}
	
	/**
	 * Operator cast to GtkWidget*.
	 */
	operator GtkWidget* () {
		return _combo;
	}
};

}

}

#endif /*TWOCOLUMNTEXTCOMBO_H_*/
