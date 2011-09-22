#include "FiltersMenu.h"

#include "i18n.h"
#include "ifilter.h"
#include "iuimanager.h"

namespace ui {

	namespace {
		// greebo: These are used for the DarkRadiant main menu
		const std::string MENU_NAME = "main";
		const std::string MENU_INSERT_BEFORE = MENU_NAME + "/map";
		const std::string MENU_FILTERS_NAME = "filters";
		const std::string MENU_PATH = MENU_NAME + "/" + MENU_FILTERS_NAME;
		const std::string MENU_ICON = "iconFilter16.png";

		// Local visitor class to populate the filters menu
		class MenuPopulatingVisitor :
			public IFilterVisitor
		{
			// The path under which the items get added.
			std::string _targetPath;
		public:
			// Pass the target menu path to the constructor
			MenuPopulatingVisitor(const std::string& targetPath) :
				_targetPath(targetPath)
			{}

			// Visitor function
			void visit(const std::string& filterName) {
				// Get the menu manager
				IMenuManager& menuManager = GlobalUIManager().getMenuManager();

				std::string eventName =
					GlobalFilterSystem().getFilterEventName(filterName);

				// Create the menu item
				menuManager.add(_targetPath, _targetPath + "_" + filterName,
								menuItem, filterName,
								MENU_ICON, eventName);
			}
		};
	}

// Construct GTK widgets
void FiltersMenu::addItemsToMainMenu()
{
	// Get the menu manager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	// remove any items first
	removeItemsFromMainMenu();

	// Create the toplevel menu item
	menuManager.insert(MENU_INSERT_BEFORE, MENU_FILTERS_NAME,
						ui::menuFolder, "Fi_lter", "", ""); // empty icon, empty event

	// Visit the filters in the FilterSystem to populate the menu
	MenuPopulatingVisitor visitor(MENU_PATH);
	GlobalFilterSystem().forEachFilter(visitor);

	menuManager.add(MENU_PATH, "_FiltersSep1", menuSeparator, "", "", "");
	menuManager.add(MENU_PATH, "ActivateAllFilters", menuItem, _("Activate _all Filters"), MENU_ICON, "ActivateAllFilters");
	menuManager.add(MENU_PATH, "DeactivateAllFilters", menuItem, _("_Deactivate all Filters"), MENU_ICON, "DeactivateAllFilters");

	menuManager.add(MENU_PATH, "_FiltersSep2", menuSeparator, "", "", "");
	menuManager.add(MENU_PATH, "EditFilters", menuItem, _("Edit Filters..."), MENU_ICON, "EditFiltersDialog");
}

void FiltersMenu::removeItemsFromMainMenu()
{
	// Get the menu manager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	// Remove the filters menu if there exists one
	menuManager.remove(MENU_PATH);
}

} // namespace ui
