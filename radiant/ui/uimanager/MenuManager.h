#ifndef MENUMANAGER_H_
#define MENUMANAGER_H_

#include <string>
#include <list>
#include "MenuItem.h"

// Forward declarations
typedef struct _GtkWidget GtkWidget;

namespace ui {

class MenuManager
{
	// The root item containing the menubars as children
	MenuItemPtr _root;	

public:
	// Constructor, initialises the menu from the registry
	MenuManager();
	
	/** greebo: Retrieves the menu with the given <name>.
	 * 
	 * @returns: the widget, or NULL, if no <name> menu has been found.
	 */
	GtkWidget* getMenu(const std::string& name);
	
	void add(const std::string& menuPath, 
			 const std::string& caption, 
			 const std::string& eventName);
	
private:
	/** greebo: Loads all the menu items from the registry
	 */		 
	void loadFromRegistry();
};

} // namespace ui

#endif /*MENUMANAGER_H_*/
