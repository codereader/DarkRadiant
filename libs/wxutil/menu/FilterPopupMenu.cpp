#include "FilterPopupMenu.h"

#include "ieventmanager.h"
#include <wx/menu.h>

#include "wxutil/menu/IconTextMenuItem.h"

namespace wxutil
{

namespace
{
	const char* const MENU_ICON = "iconFilter16.png";
}

FilterPopupMenu::FilterPopupMenu()
{
	// Visit the filters in the FilterSystem to populate the menu
	GlobalFilterSystem().forEachFilter(std::bind(&FilterPopupMenu::visitFilter, this, std::placeholders::_1));
}

FilterPopupMenu::~FilterPopupMenu()
{
	for (const auto& item : _filterItems)
	{
		GlobalEventManager().unregisterMenuItem(item.first, item.second);
	}
}

void FilterPopupMenu::visitFilter(const std::string& filterName)
{
	auto* item = Append(new wxutil::IconTextMenuItem(filterName, MENU_ICON));
	item->SetCheckable(true);

	std::string eventName = GlobalFilterSystem().getFilterEventName(filterName);

	GlobalEventManager().registerMenuItem(eventName, item);

    // We remember the item mapping for deregistration on shutdown
	_filterItems.emplace(eventName, item);
}

} // namespace
