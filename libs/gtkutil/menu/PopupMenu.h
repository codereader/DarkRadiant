#ifndef POPUPMENU_H_
#define POPUPMENU_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkmenuitem.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <list>

namespace gtkutil
{

/**
 * A free pop-up menu populated with items and displayed on demand. Useful for
 * right-click context menus.
 */
class PopupMenu
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

private:
	
	// Main menu widget
	GtkWidget* _menu;

	// Data class containing the elements of a menu item
	struct MenuItem
	{
		GtkWidget* widget;
		Callback callback;
		SensitivityTest sensitivityTest;
		VisibilityTest visibilityTest;
		
		MenuItem(GtkWidget* w, 
				 const Callback& c, 
				 const SensitivityTest& s,
				 const VisibilityTest& v)
		: widget(w), 
		  callback(c), 
		  sensitivityTest(s),
		  visibilityTest(v)
		{ }
	};
	
	// List of menu items
	typedef std::list<MenuItem> MenuItemList;
	MenuItemList _menuItems;	
	
private:
	
	/*
	 * Default test. Returns true in all cases. If a menu item does
	 * not specify its own test, this will be used to ensure the
	 * item is always visible or sensitive.
	 */
	static bool _alwaysTrue() { return true; }
	
	/* GTK CALLBACKS */
	
	// Main activation callback from GTK
	static void _onActivate(GtkMenuItem* item, MenuItem* menuItem) {
		menuItem->callback();
	}
	
	// Mouse click callback (if required)
	static gboolean _onClick(GtkWidget* w, GdkEventButton* e, PopupMenu* self);
	
public:
	
	/**
	 * Default constructor.
	 * 
	 * @param widget
	 * Optional widget for which this menu should be a right-click popup menu.
	 * If not set to NULL, the PopupMenu will connect to the 
	 * button-release-event on this widget and automatically display itself
	 * when a right-click is detected.
	 */
	PopupMenu(GtkWidget* widget = NULL);
	
	/**
	 * Destructor.
	 */
	~PopupMenu() {
		g_object_unref(_menu);
	}
	
	/**
	 * Add an item to this menu using a widget and callback function.
	 * 
	 * @param widget
	 * The GtkWidget containing the displayed menu elements (i.e. icon and
	 * text).
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
	void addItem(GtkWidget* widget,
				 const Callback& callback,
				 const SensitivityTest& sensTest = SensitivityTest(_alwaysTrue),
				 const VisibilityTest& visTest = VisibilityTest(_alwaysTrue));
	
	/**
	 * Show this menu. Each menu item's SensitivityTest will be invoked to 
	 * determine whether it should be enabled or not, then the menu will be
	 * displayed.
	 */
	void show();
};
typedef boost::shared_ptr<PopupMenu> PopupMenuPtr;

} // namespace gtkutil

#endif /*POPUPMENU_H_*/
