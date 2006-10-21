#include "FiltersMenu.h"

#include "ifilter.h"
#include "gtkutil/IconTextMenuToggle.h"
#include "gtkutil/menu.h"

#include <gtk/gtkmenu.h>

namespace ui
{

// Construct GTK widgets

FiltersMenu::FiltersMenu()
: _menu(new_sub_menu_item_with_mnemonic("Fi_lter"))
{
	// Local visitor class to populate the menu
	struct MenuPopulatingVisitor
	: public IFilterVisitor
	{
		// FiltersMenu to populate
		GtkMenu* _menu;
		
		// Constructor
		MenuPopulatingVisitor(GtkMenu* menu)
		: _menu(menu)
		{}
		
		// Visitor function
		void visit(const std::string& filterName) {
			// Create and append the menu item for this filter
			GtkWidget* filterItem = gtkutil::IconTextMenuToggle("iconFilter16.png", filterName.c_str());
			gtk_widget_show_all(filterItem);
			gtk_menu_shell_append(GTK_MENU_SHELL(_menu), filterItem);
			// Connect up the signal
			g_signal_connect(G_OBJECT(filterItem), 
							 "toggled", 
							 G_CALLBACK(_onFilterToggle),
							 const_cast<std::string*>(&filterName)); // string owned by filtersystem, GTK requires void* not const void*
		}	
	};
 
	// Visit the filters in the FilterSystem to populate the menu
	GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(_menu));
	MenuPopulatingVisitor visitor(menu);
	GlobalFilterSystem().forEachFilter(visitor);
}

/* GTK CALLBACKS */

void FiltersMenu::_onFilterToggle(GtkCheckMenuItem* item, const std::string* name) {
	GlobalFilterSystem().setFilterState(*name, gtk_check_menu_item_get_active(item));
}
 
} // namespace ui
