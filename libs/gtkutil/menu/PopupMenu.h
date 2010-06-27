#ifndef POPUPMENU_H_
#define POPUPMENU_H_

#include "imenu.h"

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include <glib/gtypes.h>

#include "MenuItem.h"

typedef struct _GdkEventButton GdkEventButton;
typedef struct _GtkMenuItem GtkMenuItem;

namespace gtkutil
{

/**
 * A free pop-up menu populated with items and displayed on demand. Useful for
 * right-click context menus.
 */
class PopupMenu :
	public ui::IMenu
{
private:
	
	// Main menu widget
	GtkWidget* _menu;

	// List of menu items
	typedef std::list<ui::IMenuItemPtr> MenuItemList;
	MenuItemList _menuItems;	
	
private:
	
	/* GTK CALLBACKS */
	
	// Main activation callback from GTK
	static void _onActivate(GtkMenuItem* item, ui::IMenuItem* menuItem);
	
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
	virtual ~PopupMenu();
	
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
	virtual void addItem(GtkWidget* widget,
						 const Callback& callback,
						 const SensitivityTest& sensTest = SensitivityTest(_alwaysTrue),
						 const VisibilityTest& visTest = VisibilityTest(_alwaysTrue));
	
	virtual void addItem(const ui::IMenuItemPtr& item);

	/**
	 * Show this menu. Each menu item's SensitivityTest will be invoked to 
	 * determine whether it should be enabled or not, then the menu will be
	 * displayed.
	 */
	virtual void show();
};
typedef boost::shared_ptr<PopupMenu> PopupMenuPtr;

} // namespace gtkutil

#endif /*POPUPMENU_H_*/
