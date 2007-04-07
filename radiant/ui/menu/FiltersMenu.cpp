#include "FiltersMenu.h"

#include "ifilter.h"
#include "iuimanager.h"

namespace ui {
	
	namespace {
		const std::string MENU_NAME = "main";
		const std::string MENU_INSERT_BEFORE = MENU_NAME + "/map";
		const std::string MENU_FILTERS_NAME = "filters";
		const std::string MENU_PATH = MENU_NAME + "/" + MENU_FILTERS_NAME;
		const std::string MENU_ICON = "iconFilter16.png";
	}

// Construct GTK widgets
void FiltersMenu::addItems() {

	// Local visitor class to populate the filters menu
	struct MenuPopulatingVisitor : 
		public IFilterVisitor
	{
		// Visitor function
		void visit(const std::string& filterName) {
			// Get the menu manager
			IMenuManager& menuManager = GlobalUIManager().getMenuManager();
	
			std::string eventName = 
				GlobalFilterSystem().getFilterEventName(filterName);

			// Create the toplevel menu item
			menuManager.add(MENU_PATH, filterName, 
							 ui::menuItem, filterName, 
							 MENU_ICON, eventName);
		}	
	};

	// Get the menu manager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();
	
	// Create the toplevel menu item
	menuManager.insert(MENU_INSERT_BEFORE, MENU_FILTERS_NAME, 
						ui::menuFolder, "Fi_lter", "", ""); // empty icon, empty event
	
	// Visit the filters in the FilterSystem to populate the menu
	MenuPopulatingVisitor visitor;
	GlobalFilterSystem().forEachFilter(visitor);
}

} // namespace ui
