#ifndef SEPARATORMENUITEM_H_
#define SEPARATORMENUITEM_H_

#include <gtk/gtkseparatormenuitem.h>
#include <string>

namespace gtkutil
{

/** Encapsulation of a GtkSeparatorMenuItem for use in a GtkMenu.
 */

class SeparatorMenuItem
{
public:

	// Constructor
	SeparatorMenuItem()
	{}
	
	// Operator cast to GtkWidget* for packing into a menu
	operator GtkWidget* () {
		GtkWidget* separatorMenuItem = gtk_separator_menu_item_new();
		return separatorMenuItem;
	}
};

}

#endif /*SEPARATORMENUITEM_H_*/
