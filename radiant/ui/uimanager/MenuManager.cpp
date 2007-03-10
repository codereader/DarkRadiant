#include "MenuManager.h"

#include "iregistry.h"

namespace ui {

	namespace {
		// The menu root key in the registry
		const std::string RKEY_MENU_ROOT = "user/ui/menu";
		const std::string TYPE_ITEM = "item";
	}

MenuManager::MenuManager() :
	_menuRoot(new MenuItem(MenuItemPtr()))
{
	globalOutputStream() << "MenuManager: Loading menu from registry.\n";
	loadFromRegistry();
	globalOutputStream() << "MenuManager: Finished loading.\n";
}

void MenuManager::loadFromRegistry() {
	xml::NodeList menus = GlobalRegistry().findXPath(RKEY_MENU_ROOT);
	
	if (menus.size() > 0) {
		_menuRoot->setName(menus[0].getAttributeValue("name"));
		// Populate the root menuitem using this node
		_menuRoot->parseNode(menus[0], _menuRoot);
	}
	else {
		globalErrorStream() << "MenuManager: Could not find menu root in registry.\n"; 
	}
}

void MenuManager::add(const std::string& menuPath, 
			 		  const std::string& caption, 
					  const std::string& eventName)
{
	MenuItemPtr found = _menuRoot->find(menuPath);
	
	if (found == NULL) {
		 
	}
	else {
		// menu item already exists
		globalErrorStream() << "MenuItem: " << menuPath.c_str() << " already exists.\n"; 
	}
}
	
} // namespace ui
