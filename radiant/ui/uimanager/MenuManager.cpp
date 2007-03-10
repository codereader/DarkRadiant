#include "MenuManager.h"

#include "iregistry.h"

namespace ui {

	namespace {
		// The menu root key in the registry
		const std::string RKEY_MENU_ROOT = "user/ui/menu/";
	}

MenuManager::MenuManager() {
	
}

void MenuManager::add(const std::string& menuPath, 
			 		  const std::string& caption, 
					  const std::string& eventName)
{
	if (!GlobalRegistry().keyExists(menuPath)) {
		// Create the key
		xml::Node menuItem(GlobalRegistry().createKey(menuPath));
		
	}
	else {
		// menu item already exists
		globalErrorStream() << "MenuItem: " << menuPath.c_str() << " already exists.\n"; 
	}
}
	
} // namespace ui
