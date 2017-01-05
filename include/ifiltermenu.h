#pragma once

#include <string>
#include <memory>

class wxMenu;

namespace ui
{

/**
 * A class representing a Filters submenu, with a proper
 * getWidget() method for packing it into a parent container.
 *
 * It's the caller's responsibility to delete the object.
 *
 * Use the GlobalUIManager() interface to acquire
 * a new instance of this filter menu.
 */
class IFilterMenu
{
public:
	virtual ~IFilterMenu() {}

	// Constructs and returns the widget of a full filters menu
	// including submenu and the items. This can be packed into an
	// existing menu bar or toolitem right away.
	// Caller is responsible of deleting the menu!
	virtual wxMenu* getMenuWidget() = 0;
};
typedef std::shared_ptr<IFilterMenu> IFilterMenuPtr;

} // namespace ui
