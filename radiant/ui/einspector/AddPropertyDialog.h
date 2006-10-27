#ifndef ADDPROPERTYDIALOG_H_
#define ADDPROPERTYDIALOG_H_

#include <gtk/gtkwidget.h>

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
	
	// The selected property
	std::string _selectedProperty;
	
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
