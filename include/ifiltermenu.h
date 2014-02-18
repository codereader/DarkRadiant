#pragma once

#include <string>
#include <boost/shared_ptr.hpp>

class wxMenu;

namespace ui
{

/**
 * A class representing a Filters submenu, with a proper
 * getWidget() method for packing it into a parent container.
 *
 * Upon construction, the menu will be registered in the
 * global MenuManager. The destructor will remove it from there,
 * so the client needs to prevent this class from getting
 * out of scope too soon.
 *
 * Use the GlobalUIManager() interface to acquire
 * a new instance of this filter menu.
 */
class IFilterMenu
{
public:
	virtual ~IFilterMenu() {}

	// Constructs and returns the widget of a full filters menu bar
	// including submenu and the items. This can be packed into a
	// parent sizer or other wxWidget containers right away.
	virtual wxMenu* getMenuBarWidget() = 0;
};
typedef boost::shared_ptr<IFilterMenu> IFilterMenuPtr;

} // namespace ui
