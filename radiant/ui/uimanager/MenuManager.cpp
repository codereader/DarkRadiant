#include "MenuManager.h"

#include "iregistry.h"
#include <gtk/gtkwidget.h>
#include <gtk/gtkmenushell.h>
#include <gtk/gtkmenuitem.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

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
	loadFromRegistry();
	globalOutputStream() << "MenuManager: Finished loading default menu from registry.\n";
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

GtkWidget* MenuManager::add(const std::string& insertPath,
							const std::string& name,
					  		eMenuItemType type,
			 		  		const std::string& caption, 
			 		  		const std::string& icon,
					  		const std::string& eventName)
{
	MenuItemPtr found = _root->find(insertPath);
	
	if (found != NULL) {
		// Allocate a new MenuItem
		MenuItemPtr newItem = MenuItemPtr(new MenuItem(found));
		found->addChild(newItem);
		
		newItem->setName(name);
		newItem->setCaption(caption);
		newItem->setType(type);
		newItem->setIcon(icon);
		newItem->setEvent(eventName);
		
		// Cast the parent onto a GtkWidget* (a menu item)
		GtkWidget* parentItem = *found;
		// Retrieve the submenu widget from the item
		GtkWidget* parent = gtk_menu_item_get_submenu(GTK_MENU_ITEM(parentItem));
		//GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(_menu));
		gtk_menu_shell_append(GTK_MENU_SHELL(parent), *newItem);
		
		return *newItem;
	}
	else {
		globalErrorStream() << "MenuItem: " << insertPath.c_str() << " already exists.\n"; 
	}
	return NULL;
}

GtkWidget* MenuManager::insert(const std::string& insertPath,
						 const std::string& name,
						 eMenuItemType type,
						 const std::string& caption,
						 const std::string& icon,
						 const std::string& eventName)
{
	MenuItemPtr found = _root->find(insertPath);
	
	if (found != NULL) {
		if (found->parent() != NULL) {
			// Get the GTK Menu position of the child widget
			int position = found->parent()->getMenuPosition(found);
			// Allocate a new MenuItem
			MenuItemPtr newItem = MenuItemPtr(new MenuItem(found->parent()));
			found->parent()->addChild(newItem);
			
			// Load the properties into the new child
			newItem->setName(name);
			newItem->setType(type);
			newItem->setCaption(caption);
			newItem->setEvent(eventName);
			newItem->setIcon(icon);
			
			GtkWidget* parentWidget = *found->parent();
			
			// Insert it at the given position
			if (found->parent()->getType() == menuBar) {
				// The parent is a menubar, it's a menushell in the first place
				gtk_menu_shell_insert(GTK_MENU_SHELL(parentWidget), *newItem, position);
			}
			else if (found->parent()->getType() == menuFolder) {
				// The parent is a submenu (=menuitem), try to retrieve the menushell first
				GtkWidget* subMenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(parentWidget));
				gtk_menu_shell_insert(GTK_MENU_SHELL(subMenu), *newItem, position);
			}
			
			return *newItem;
		}
		else {
			globalErrorStream() << "MenuManager: Unparented menuitem, can't determine position: ";
			globalErrorStream() << insertPath.c_str() << "\n";
			return NULL;
		}
	}
	else {
		globalErrorStream() << "MenuManager: Could not find insertPath: " << insertPath.c_str() << "\n";
		return NULL; 
	}
}

} // namespace ui
