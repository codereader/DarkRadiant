#ifndef MENUMANAGER_H_
#define MENUMANAGER_H_

#include <string>
#include <list>
#include "iuimanager.h"
#include "MenuItem.h"

// Forward declarations
typedef struct _GtkWidget GtkWidget;

namespace ui {

class MenuManager :
	public IMenuManager
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
	
	GtkWidget* add(const std::string& insertPath,
				   const std::string& name,
			 	   eMenuItemType type, 
				   const std::string& caption,
				   const std::string& icon,
				   const std::string& eventName);
	
	/** greebo: Inserts a new menuItem as sibling _before_ the given insertPath.
	 * 
	 * @name: the name of the new menu item (no path, just the name)
	 * @caption: the display string including mnemonic
	 * @eventName: the event name this item is associated with (can be empty). 
	 */	 
	GtkWidget* insert(const std::string& insertPath,
					  const std::string& name,
					  eMenuItemType type,
					  const std::string& caption,
					  const std::string& icon,
					  const std::string& eventName);
	
private:
	/** greebo: Loads all the menu items from the registry
	 */		 
	void loadFromRegistry();
};

} // namespace ui

#endif /*MENUMANAGER_H_*/
