#pragma once

#include "ui/imenu.h"

#include <functional>
#include <memory>
#include "util/Noncopyable.h"
#include <vector>

#include <wx/menu.h>

#include "MenuItem.h"

namespace wxutil
{

/**
 * A free pop-up menu populated with items and displayed on demand. Useful for
 * right-click context menus.
 */
class PopupMenu :
	public wxMenu,
	public ui::IMenu
{
private:
	// List of menu items
	typedef std::vector<ui::IMenuItemPtr> MenuItemList;
	MenuItemList _menuItems;

private:

	// Mouse click callback (if required)
	void _onItemClick(wxCommandEvent& ev);

public:

	/**
	 * Default constructor.
	 */
	PopupMenu();

	/**
	 * Destructor.
	 */
	virtual ~PopupMenu();

	/**
	 * Add an item to this menu using a widget and callback function.
	 *
	 * @param widget
	 * The menu item containing the displayed elements (i.e. icon and text).
	 *
	 * @param callback
	 * A callback function to be invoked when this menu item is activated.
	 *
	 * @param sensTest
	 * SensitivityTest function object to determine whether this menu item is
	 * currently clickable (optional).
	 *
	 * @param visTest
	 * VisibilityTest function object to determine whether this menu item is
	 * currently visible (optional).
	 */
	virtual void addItem(wxMenuItem* widget,
						 const Callback& callback,
						 const SensitivityTest& sensTest = SensitivityTest(_alwaysTrue),
						 const VisibilityTest& visTest = VisibilityTest(_alwaysTrue));

	virtual void addItem(const ui::IMenuItemPtr& item);

	// Adds a separator item (horizontal line)
	virtual void addSeparator();

	/**
	 * Show this menu. Each menu item's SensitivityTest will be invoked to
	 * determine whether it should be enabled or not, then the menu will be
	 * displayed.
	 */
	virtual void show(wxWindow* parent);

protected:
	virtual void foreachMenuItem(const std::function<void(const ui::IMenuItemPtr&)>& functor);

    // Returns true if the given item is visible for the projected menuPosition
    virtual bool itemIsVisible(int index, int menuPosition);

private:
    void attachItem(const ui::IMenuItemPtr& item, int position);
    void detachItem(const ui::IMenuItemPtr& item);
};
typedef std::shared_ptr<PopupMenu> PopupMenuPtr;

} // namespace
