#include "FiltersMainMenu.h"

#include "i18n.h"
#include "ifilter.h"
#include "ui/imenumanager.h"

namespace ui 
{

	namespace 
	{
		// greebo: These are used for the DarkRadiant main menu
		const std::string MENU_NAME = "main";
		const std::string MENU_INSERT_BEFORE = MENU_NAME + "/map";
		const std::string MENU_FILTERS_NAME = "filters";
		const std::string MENU_PATH = MENU_NAME + "/" + MENU_FILTERS_NAME;
		const std::string MENU_ICON = "iconFilter16.png";
	}

// Construct menu items
void FiltersMenu::addItemsToMainMenu()
{
	// Get the menu manager
	menu::IMenuManager& menuManager = GlobalMenuManager();

	// remove any items first
	removeItemsFromMainMenu();

	// Create the toplevel menu item
	menuManager.insert(MENU_INSERT_BEFORE, MENU_FILTERS_NAME,
						menu::ItemType::Folder, "Fi&lter", "", ""); // empty icon, empty event

	// Visit the filters in the FilterSystem to populate the menu
	GlobalFilterSystem().forEachFilter([&](const std::string& filterName)
	{
		std::string eventName = GlobalFilterSystem().getFilterEventName(filterName);

		// Create the menu item
		menuManager.add(MENU_PATH, MENU_PATH + "_" + filterName,
            menu::ItemType::Item, filterName,
			MENU_ICON, eventName);
	});

	menuManager.add(MENU_PATH, "_FiltersSep1", menu::ItemType::Separator, "", "", "");
	menuManager.add(MENU_PATH, "ActivateAllFilters", menu::ItemType::Item, _("Activate &all Filters"), MENU_ICON, "ActivateAllFilters");
	menuManager.add(MENU_PATH, "DeactivateAllFilters", menu::ItemType::Item, _("&Deactivate all Filters"), MENU_ICON, "DeactivateAllFilters");

	menuManager.add(MENU_PATH, "_FiltersSep2", menu::ItemType::Separator, "", "", "");
	menuManager.add(MENU_PATH, "EditFilters", menu::ItemType::Item, _("Edit Filters..."), MENU_ICON, "EditFiltersDialog");
}

void FiltersMenu::removeItemsFromMainMenu()
{
	// Remove the filters menu if there exists one
    GlobalMenuManager().remove(MENU_PATH);
}

} // namespace ui
