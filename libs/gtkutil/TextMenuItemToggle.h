#ifndef TEXTMENUITEMTOGGLE_H_
#define TEXTMENUITEMTOGGLE_H_

#include "TextMenuItem.h"

#include "gtk/gtkcheckmenuitem.h"

namespace gtkutil
{

/** Encapsulation of a GtkCheckMenuItem for use in a GtkMenu.
 */
class TextMenuItemToggle :
	public TextMenuItemMnemonic
{
public:

	// Constructor
	TextMenuItemToggle(const std::string& text)
		: TextMenuItemMnemonic(text)
	{}
	
	// Operator cast to GtkWidget* for packing into a menu
	virtual operator GtkWidget* () {
		GtkWidget* menuItem = gtk_check_menu_item_new_with_mnemonic(_label.c_str());
		return menuItem;
	}
};

} // namespace gtkutil

#endif /*TEXTMENUITEMTOGGLE_H_*/
