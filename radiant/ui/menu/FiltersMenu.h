#ifndef FILTERSMENU_H_
#define FILTERSMENU_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkcheckmenuitem.h>

#include <string>

namespace ui
{

/** Utility class for generating the Filters top-level menu. This class
 * constructs the relevant widgets on instantiation, and then passes the
 * widgets up to GTK to manage.
 */

class FiltersMenu
{
	// Main menu item
	GtkMenuItem* _menu;
	
private:

	/* GTK CALLBACKS */
	static void _onFilterToggle(GtkCheckMenuItem* item, const std::string* name);
		
public:

	/** Construct the Filters menu.
	 */
	FiltersMenu();
	
	/** Operator cast to GtkMenuItem* for packing into the main menu
	 * bar.
	 */
	operator GtkWidget* () {
		return GTK_WIDGET(_menu);
	}
};

}

#endif /*FILTERSMENU_H_*/
