#include "FilterMenu.h"

#include "iuimanager.h"
#include "ieventmanager.h"
#include <wx/menu.h>

#include "wxutil/menu/IconTextMenuItem.h"

namespace ui
{

namespace
{
	const char* const MENU_ICON = "iconFilter16.png";
}

FilterMenu::FilterMenu() :
	_menu(new wxutil::PopupMenu)
{
	// Visit the filters in the FilterSystem to populate the menu
	GlobalFilterSystem().forEachFilter(*this);
}

FilterMenu::~FilterMenu()
{
	for (auto i : _filterItems)
	{
		IEventPtr event = GlobalEventManager().findEvent(i.first);

		if (event)
		{
			event->disconnectMenuItem(i.second);
		}
	}

	_menu = nullptr;
}

void FilterMenu::visit(const std::string& filterName)
{
	wxMenuItem* item = _menu->Append(new wxutil::IconTextMenuItem(filterName, MENU_ICON));
	item->SetCheckable(true);

	std::string eventName = GlobalFilterSystem().getFilterEventName(filterName);

	IEventPtr event = GlobalEventManager().findEvent(eventName);

	if (event)
	{
		event->connectMenuItem(item);
	}

	_filterItems.insert(std::make_pair(eventName, item));
}

wxMenu* FilterMenu::getMenuWidget()
{
	return _menu;
}

} // namespace
