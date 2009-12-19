#ifndef FILTERSMENU_H_
#define FILTERSMENU_H_

#include <string>

typedef struct _GtkWidget GtkWidget;

namespace ui {

/** Utility class for generating the Filters top-level menu. This class
 * registers the relevant menuitems on demand.
 * 
 * Construct a FiltersMenu instance to generate a new Filter Menu which
 * can be packed into a parent container widget using the GtkWidget* operator.
 */
class FiltersMenu
{
	GtkWidget* _menu;

	// Static counter to create unique menu bar widgets
	static int _counter;

	// The path of this menu
	std::string _path;

public:
	// Constructs the filters submenu which can be added to a menubar
	FiltersMenu();

	~FiltersMenu();

	// Returns a GtkWidget* with a fabricated filters submenu,
	// ready for packing into a menu bar.
	operator GtkWidget*();

	/** Public service method. Adds the menuitems to the global Menu.
	 *  Should be called by the Mainframe window only (and only once).
	 */
	static void addItemsToMainMenu();

	/**
	 * Removes all filter menu items from the menu.
	 */
	static void removeItemsFromMainMenu();
};

}

#endif /*FILTERSMENU_H_*/
