#include "FiltersMenu.h"

#include "ifilter.h"
#include "gtkutil/IconTextMenuToggle.h"
#include "gtkutil/menu.h"

#include "iuimanager.h"
#include <gtk/gtkmenu.h>

namespace ui {
	
	namespace {
		const std::string MENU_NAME = "main";
		const std::string MENU_INSERT_BEFORE = MENU_NAME + "/map";
		const std::string MENU_FILTERS_NAME = "filters";
		const std::string MENU_PATH = MENU_NAME + "/" + MENU_FILTERS_NAME;
		const std::string MENU_ICON = "iconFilter16.png";
	}

// Construct GTK widgets

FiltersMenu::FiltersMenu()
: _menu(new_sub_menu_item_with_mnemonic("Fi_lter"))
{
	// Local visitor class to populate the filters menu
	struct MenuPopulatingVisitor : 
		public IFilterVisitor
	{
		// Visitor function
		void visit(const std::string& filterName) {
			// Get the menu manager
			IMenuManager* menuManager = GlobalUIManager().getMenuManager();
	
			std::string eventName = GlobalFilterSystem().getFilterEventName(filterName);

			// Create the toplevel menu item
			GtkWidget* filterItem = 
				menuManager->add(MENU_PATH, filterName, 
								 ui::menuItem, filterName, 
								 MENU_ICON, eventName);
			
			// Create and append the menu item for this filter
			/*GtkWidget* filterItem = gtkutil::IconTextMenuToggle("iconFilter16.png", filterName.c_str());
			gtk_widget_show_all(filterItem);
			gtk_menu_shell_append(GTK_MENU_SHELL(_menu), filterItem);
			// Connect up the signal
			g_signal_connect(G_OBJECT(filterItem), 
							 "toggled", 
							 G_CALLBACK(_onFilterToggle),
							 const_cast<std::string*>(&filterName)); // string owned by filtersystem, GTK requires void* not const void**/
		}	
	};

	// Get the menu manager
	IMenuManager* menuManager = GlobalUIManager().getMenuManager();
	
	// Create the toplevel menu item
	GtkWidget* menu = 
		menuManager->insert(MENU_INSERT_BEFORE, MENU_FILTERS_NAME, 
							ui::menuFolder, "_Filter", "", ""); // empty icon, empty event
	
	// Visit the filters in the FilterSystem to populate the menu
	//GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(_menu));
	MenuPopulatingVisitor visitor;
	GlobalFilterSystem().forEachFilter(visitor);
}

/* GTK CALLBACKS */
/*
void FiltersMenu::_onFilterToggle(GtkCheckMenuItem* item, const std::string* name) {
	GlobalFilterSystem().setFilterState(*name, gtk_check_menu_item_get_active(item));
}*/
 
} // namespace ui
