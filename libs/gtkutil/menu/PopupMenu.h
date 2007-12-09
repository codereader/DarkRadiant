#ifndef POPUPMENU_H_
#define POPUPMENU_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkmenuitem.h>
#include <boost/function.hpp>
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
	 * indicating whether the associated menu item should be visible or not.
	 */
	typedef boost::function<bool (void)> SensitivityTest;

private:
	
	// Main menu widget
	GtkWidget* _menu;

	// Data class containing the elements of a menu item
	struct MenuItem {
		GtkWidget* widget;
		Callback callback;
		SensitivityTest test;
		
		MenuItem(GtkWidget* w, Callback c, SensitivityTest t)
		: widget(w), callback(c), test(t)
		{ }
	};
	
	// List of menu items
	typedef std::list<MenuItem> MenuItemList;
	MenuItemList _menuItems;	
	
private:
	
	/*
	 * Default sensitivity test. Return true in all cases. If a menu item does
	 * not specify its own sensitivity test, this will be used to ensure the
	 * item is always visible.
	 */
	static bool _alwaysVisible() { return true; }
	
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
	 * @param test
	 * SensitivityTest function object to determine whether this menu item is
	 * currently visible.
	 */
	void addItem(GtkWidget* widget,
				 Callback callback,
				 SensitivityTest test = SensitivityTest(_alwaysVisible));
	
	/**
	 * Show this menu. Each menu item's SensitivityTest will be invoked to 
	 * determine whether it should be enabled or not, then the menu will be
	 * displayed.
	 */
	void show();
};



}

#endif /*POPUPMENU_H_*/
