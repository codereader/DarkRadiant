#include "FilterOrthoContextMenuItem.h"

#include "ifilter.h"
#include "icommandsystem.h"
#include "filters/BasicFilterSystem.h"

namespace ui
{

namespace
{
	const char* const FILTER_ICON = "iconFilter16.png";
}

FilterOrthoContextMenuItem::FilterOrthoContextMenuItem(const std::string& caption,
	const FilterContextMenu::OnSelectionFunc& callback) :
	wxutil::IconTextMenuItem(caption, FILTER_ICON),
	_func(callback)
{
	// Re-populate the submenu
	_submenu = new FilterContextMenu(_func);

	// wxWidgets will take care of deleting the submenu
	SetSubMenu(_submenu);
}

FilterOrthoContextMenuItem::~FilterOrthoContextMenuItem()
{
	if (GetMenu() != nullptr)
	{
		// Destroying a menu item doesn't de-register it from the parent menu
		// To prevent double-deletions, we de-register the item on our own
		GetMenu()->Remove(GetId());
	}
}

wxMenuItem* FilterOrthoContextMenuItem::getMenuItem()
{
	return this;
}

void FilterOrthoContextMenuItem::execute()
{}

bool FilterOrthoContextMenuItem::isSensitive()
{
	return true;
}

void FilterOrthoContextMenuItem::preShow()
{
	// Re-populate the submenu
	_submenu->populate();
}

void FilterOrthoContextMenuItem::SelectByFilter(const std::string& filterName)
{
	GlobalCommandSystem().executeCommand(filters::SELECT_OBJECTS_BY_FILTER_CMD, cmd::Argument(filterName));
}

void FilterOrthoContextMenuItem::DeselectByFilter(const std::string& filterName)
{
	GlobalCommandSystem().executeCommand(filters::DESELECT_OBJECTS_BY_FILTER_CMD, cmd::Argument(filterName));
}

} // namespace
