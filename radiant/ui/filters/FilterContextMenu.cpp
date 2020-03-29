#include "FilterContextMenu.h"

#include "iuimanager.h"
#include "imap.h"
#include "wxutil/menu/IconTextMenuItem.h"

namespace ui
{

namespace
{
	const std::string FILTER_ICON("iconFilter16.png");
}

FilterContextMenu::FilterContextMenu(OnSelectionFunc& onSelection) :
	wxMenu(),
	_onSelection(onSelection)
{}

void FilterContextMenu::populate()
{
	// Clear all existing items
	for (const MenuItemIdToLayerMapping::value_type& i : _menuItemMapping)
	{
		wxMenu* parentMenu = GetParent();

#ifdef __WXGTK__
		// wxMSW and wxGTK are doing it differently (see comments in createMenuItems() below)
		parentMenu = this;
#endif

		if (parentMenu != nullptr)
		{
			parentMenu->Unbind(wxEVT_MENU, &FilterContextMenu::onActivate, this, i.first);
		}

		Delete(i.first);
	}

	_menuItemMapping.clear();

	// Populate the map with all layer names and IDs
	GlobalFilterSystem().forEachFilter(*this);
}

void FilterContextMenu::visit(const std::string& filterName)
{
	// Create a new menuitem
	wxMenuItem* menuItem = new wxutil::IconTextMenuItem(filterName, FILTER_ICON);

	// Add it to the parent menu
	Append(menuItem);

	// remember the filter name for this item
	_menuItemMapping[menuItem->GetId()] = filterName;

	wxMenu* parentMenu = GetParent();

#ifdef __WXGTK__
	// wxMSW and wxGTK are doing it differently (which is always great):
	// in MSW the parent menu (of this class) is firing the events, whereas
	// in GTK+ the submenu (this class) itself is doing that. So let's do an IFDEF
	parentMenu = this;
#endif

	// If we're packed to a parent menu (ourselves acting as submenu), connect the event to the parent
	if (parentMenu != nullptr)
	{
		parentMenu->Bind(wxEVT_MENU, &FilterContextMenu::onActivate, this, menuItem->GetId());
	}
}

void FilterContextMenu::onActivate(wxCommandEvent& ev)
{
	assert(_menuItemMapping.find(ev.GetId()) != _menuItemMapping.end());

	// Pass the call to the function
	_onSelection(_menuItemMapping[ev.GetId()]);
}

}
