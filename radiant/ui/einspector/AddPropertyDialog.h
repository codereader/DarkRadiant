#ifndef ADDPROPERTYDIALOG_H_
#define ADDPROPERTYDIALOG_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreestore.h>

#include <string>

namespace ui
{

/** Modal dialog to display a list of known properties and allow the user
 * to choose one. The dialog is displayed via a single static method which
 * creates the dialog, blocks in a recursive main loop until the choice is
 * made, and then returns the string property that was selected.
 */

class AddPropertyDialog
{
	// Main dialog widget
	GtkWidget* _widget;
	
	// Tree view and model
	GtkTreeStore* _treeStore;
	GtkWidget* _treeView;
	
	// The selected property
	std::string _selectedProperty;
	
private:

	// Create GUI components
	GtkWidget* createTreeView();
	GtkWidget* createButtonsPanel();
	
	// Populate tree view with properties
	void populateTreeView();

	/* GTK CALLBACKS */
	
	static void _onDelete(GtkWidget*, GdkEvent*, AddPropertyDialog*);
	
public:

	/** Constructor creates the dialog widgets.
	 */
	AddPropertyDialog();
	
	/** Static method to display an AddPropertyDialog and return the
	 * chosen property.
	 * 
	 * @returns
	 * String name of the chosen property (e.g. "light_radius").
	 */
	static std::string chooseProperty();

	/** Show this AddPropertyDialog and block in a gtk_main loop until
	 * a selection is made.
	 * 
	 * @returns
	 * The string property that was selected.
	 */
	const std::string& showAndBlock();
};

}

#endif /*ADDPROPERTYDIALOG_H_*/
