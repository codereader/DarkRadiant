#include "MenuManager.h"

#include "iregistry.h"
#include <gtk/gtkwidget.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <iostream>

namespace ui {

	namespace {
		// The menu root key in the registry
		const std::string RKEY_MENU_ROOT = "user/ui/menu";
		const std::string TYPE_ITEM = "item";
		typedef std::vector<std::string> StringVector;
	}

MenuManager::MenuManager() :
	_root(new MenuItem(MenuItemPtr())) // Allocate the root item (type is set automatically)
{
	globalOutputStream() << "MenuManager: Loading menu from registry.\n";
	loadFromRegistry();
	globalOutputStream() << "MenuManager: Finished loading.\n";
}

void MenuManager::loadFromRegistry() {
	xml::NodeList menuNodes = GlobalRegistry().findXPath(RKEY_MENU_ROOT);
	
	if (menuNodes.size() > 0) {
		for (unsigned int i = 0; i < menuNodes.size(); i++) {
			std::string name = menuNodes[i].getAttributeValue("name");
		
			// Allocate a new MenuItem with a NULL parent (i.e. root MenuItem)
			MenuItemPtr menubar = MenuItemPtr(new MenuItem(_root));
			menubar->setName(name);
		
			// Populate the root menuitem using the current node
			menubar->parseNode(menuNodes[i], menubar);
			
			// Add the menubar as child of the root (child is already parented to _root)
			_root->addChild(menubar);
		}
	}
	else {
		globalErrorStream() << "MenuManager: Could not find menu root in registry.\n"; 
	}
}

GtkWidget* MenuManager::getMenu(const std::string& name) {
	MenuItemPtr foundMenu = _root->find(name);
	
	if (foundMenu != NULL) {
		// Cast the menubar onto a GtkWidget* and return
		return *foundMenu;
	}
	else {
		globalErrorStream() << "MenuManager: Warning: Menu " << name.c_str() << " not found!\n";
		return NULL;
	}
}

void MenuManager::add(const std::string& menuPath, 
			 		  const std::string& caption, 
					  const std::string& eventName)
{
	MenuItemPtr found = _root->find(menuPath);
	
	if (found == NULL) {
		// Insert the menu item
	}
	else {
		globalErrorStream() << "MenuItem: " << menuPath.c_str() << " already exists.\n"; 
	}
}
	
} // namespace ui
