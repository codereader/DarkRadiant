#ifndef MENUMANAGER_H_
#define MENUMANAGER_H_

#include <string>

namespace ui {

class MenuManager
{
	
public:
	// Constructor, initialises the menu from the registry
	MenuManager();
	
	void add(const std::string& menuPath, 
			 const std::string& caption, 
			 const std::string& eventName);
};

} // namespace ui

#endif /*MENUMANAGER_H_*/
