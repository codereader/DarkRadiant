#ifndef _IMENU_H_
#define _IMENU_H_

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

namespace Gtk
{
	class MenuItem;
}

namespace ui
{

/**
 * An abstract MenuItem needs to provide a getWidget() method
 * which will be packed into a parent GTK container, plus
 * various callbacks for determining the visibility/sensitivity of
 * this item (these are invoked before the menu is showing itself).
 *
 * An utility implementation of this class can be found in
 * gtkutil::MenuItem which can be constructed from strings
 * and boost::function objects.
 */
class IMenuItem
{
public:
	/**
	 * Each menu item must return a distinct widget which is packed
	 * into the parent GTK container.
	 */
	virtual Gtk::MenuItem* getWidget() = 0;

	// Callback to run when this item is selected in the menus
	virtual void execute() = 0;

	/**
	 * Returns TRUE if this item is visible and should be rendered.
	 * Default implementation returns true for convenience.
	 */
	virtual bool isVisible()
	{
		return true;
	}

	/**
	 * Returns TRUE if this item is sensitive and therefore clickable.
	 * Default implementation returns true for convenience.
	 */
	virtual bool isSensitive()
	{
		return true;
	}

	/**
	 * Called to let the item prepare its visual appearance. Empty default impl.
	 */
	virtual void preShow()
	{}
};
typedef boost::shared_ptr<IMenuItem> IMenuItemPtr;

/**
 * An abstract menu object, which can have one or more IMenuItems as children.
 * The order in which the items are added is visually preserved.
 */
class IMenu
{
public:
	/* PUBLIC TYPES */

	/**
	 * Function callback. Each menu item is associated with one of these, which
	 * is invoked when the menu item is activated.
	 */
	typedef boost::function<void (void)> Callback;

	/**
	 * Sensitivity callback. This function object returns a true or false value,
	 * indicating whether the associated menu item should be clickable or not.
	 */
	typedef boost::function<bool (void)> SensitivityTest;

	/**
	 * Visibility callback. This function object returns a true or false value,
	 * indicating whether the associated menu item should be visible or not.
	 */
	typedef boost::function<bool (void)> VisibilityTest;

protected:
	/*
	 * Default test. Returns true in all cases. If a menu item does
	 * not specify its own test, this will be used to ensure the
	 * item is always visible or sensitive.
	 */
	static bool _alwaysTrue() { return true; }

public:
	// Convenience method, directly taking text and icon strings plus
	// callback function objects as argument.
	virtual void addItem(Gtk::MenuItem* widget,
						 const Callback& callback,
						 const SensitivityTest& sensTest = SensitivityTest(_alwaysTrue),
						 const VisibilityTest& visTest = VisibilityTest(_alwaysTrue)) = 0;

	// Adds a certain item to this menu
	virtual void addItem(const IMenuItemPtr& item) = 0;
};

} // namespace

#endif /* _IMENU_H_ */
