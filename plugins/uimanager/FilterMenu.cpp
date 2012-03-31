#include "FilterMenu.h"

#include "iuimanager.h"
#include "string/convert.h"
#include "i18n.h"

namespace ui
{

namespace
{
	// These are used for the general-purpose Filter Menu:
	const std::string FILTERS_MENU_BAR = "filters";
	const std::string FILTERS_MENU_FOLDER = "allfilters";
	const char* const FILTERS_MENU_CAPTION = N_("_Filters");

	const std::string MENU_ICON = "iconFilter16.png";
}

std::size_t FilterMenu::_counter = 0;

FilterMenu::FilterMenu()
{
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	// Create a unique name for the menu
	_path = FILTERS_MENU_BAR + string::to_string(_counter++);

	// Menu not yet constructed, do it now
	// Create the menu bar first
	_menu = menuManager.add("", _path, menuBar, _("Filters"), "", "");

	// Create the folder as child of the bar
	menuManager.add(_path, FILTERS_MENU_FOLDER,
					menuFolder, _(FILTERS_MENU_CAPTION), "", "");

	_targetPath = _path + "/" + FILTERS_MENU_FOLDER;

	// Visit the filters in the FilterSystem to populate the menu
	GlobalFilterSystem().forEachFilter(*this);
}

FilterMenu::~FilterMenu()
{
	GlobalUIManager().getMenuManager().remove(_path);
}

// Visitor function
void FilterMenu::visit(const std::string& filterName)
{
	// Get the menu manager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	std::string eventName = GlobalFilterSystem().getFilterEventName(filterName);

	// Create the menu item
	menuManager.add(_targetPath, _targetPath + "_" + filterName,
					menuItem, filterName,
					MENU_ICON, eventName);
}

Gtk::Widget* FilterMenu::getMenuBarWidget()
{
	return _menu;
}

} // namespace
