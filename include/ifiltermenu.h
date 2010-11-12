#ifndef _IFILTERMENU_H_
#define _IFILTERMENU_H_

#include <string>
#include <boost/shared_ptr.hpp>

namespace Gtk { class Widget; }

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
	// parent hbox or other GTK containers right away.
	virtual Gtk::Widget* getMenuBarWidget() = 0;
};
typedef boost::shared_ptr<IFilterMenu> IFilterMenuPtr;

} // namespace ui

#endif /* _IFILTERMENU_H_ */
