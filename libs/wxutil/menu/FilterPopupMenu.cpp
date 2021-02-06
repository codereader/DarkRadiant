#include "FilterPopupMenu.h"

#include "iuimanager.h"
#include "ieventmanager.h"
#include <wx/menu.h>

#include "wxutil/menu/IconTextMenuItem.h"

namespace wxutil
{

namespace
{
	const char* const MENU_ICON = "iconFilter16.png";
}

FilterPopupMenu::FilterPopupMenu() :
	_menu(new wxutil::PopupMenu)
{
	// Visit the filters in the FilterSystem to populate the menu
	GlobalFilterSystem().forEachFilter(std::bind(&FilterPopupMenu::visitFilter, this, std::placeholders::_1));
}

FilterPopupMenu::~FilterPopupMenu()
{
	for (const auto& i : _filterItems)
	{
		GlobalEventManager().unregisterMenuItem(i.first, i.second);
	}

	_menu = nullptr;
}

void FilterPopupMenu::visitFilter(const std::string& filterName)
{
	wxMenuItem* item = _menu->Append(new wxutil::IconTextMenuItem(filterName, MENU_ICON));
	item->SetCheckable(true);

	std::string eventName = GlobalFilterSystem().getFilterEventName(filterName);

	GlobalEventManager().registerMenuItem(eventName, item);

	_filterItems.emplace(eventName, item);
}

wxMenu* FilterPopupMenu::getMenuWidget()
{
	return _menu;
}

} // namespace
