#ifndef MENUMANAGER_H_
#define MENUMANAGER_H_

#include <string>
#include <list>
#include "MenuItem.h"

namespace ui {

class MenuManager
{
	MenuItemPtr _menuRoot;
public:
	// Constructor, initialises the menu from the registry
	MenuManager();
	
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
