#include "MenuItem.h"

#include "ieventmanager.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include "gtkutil/MenuItemAccelerator.h"
#include <gtk/gtk.h>

#include "ui/mru/MRU.h"
#include <iostream>

namespace ui {
	
	namespace {
		typedef std::vector<std::string> StringVector;
	}

MenuItem::MenuItem(MenuItemPtr parent) :
	_parent(parent),
	_widget(NULL),
	_type(menuNothing),
	_constructed(false)
{
	if (_parent == NULL) {
		_type = menuRoot;
	}
	else if (_parent->isRoot()) {
		_type = menuBar;
	}
}

std::string MenuItem::getName() const {
	return _name;
}

void MenuItem::setName(const std::string& name) {
	_name = name;
}

bool MenuItem::isRoot() const {
	return (_type == menuRoot);
}

MenuItemPtr MenuItem::parent() const {
	return _parent;
}

void MenuItem::setParent(MenuItemPtr parent) {
	_parent = parent;
}
	
void MenuItem::setCaption(const std::string& caption) {
	_caption = caption;
}

std::string MenuItem::getCaption() const {
	return _caption;
}

void MenuItem::setIcon(const std::string& icon) {
	_icon = icon;
}

bool MenuItem::isEmpty() const {
	return (_type != menuItem);
}

eMenuItemType MenuItem::getType() const {
	return _type;
}

void MenuItem::setType(eMenuItemType type) {
	_type = type;
}

int MenuItem::numChildren() const {
	return _children.size();
}

void MenuItem::addChild(MenuItemPtr newChild) {
	_children.push_back(newChild);
}

std::string MenuItem::getEvent() const {
	return _event;
}

void MenuItem::setEvent(const std::string& eventName) {
	_event = eventName;
}

int MenuItem::getMenuPosition(MenuItemPtr child) {
	if (!_constructed) {
		construct();
	}
	
	// Check if this is the right item type for this operation
	if (_type == menuFolder || _type == menuBar) {
		// Get the list of child widgets
		GList* gtkChildren = gtk_container_get_children(GTK_CONTAINER(_widget));
		
		// Cast the child onto a GtkWidget for comparison
		GtkWidget* childWidget = *child;
		
		int index = 0;
		while (gtkChildren != NULL) {
			// Get the widget pointer from the current list item
			GtkWidget* candidate = reinterpret_cast<GtkWidget*>(gtkChildren->data);
			
			// Have we found the widget?
			if (candidate == childWidget) {
				return index;
			}
			
			index++;
			gtkChildren = gtkChildren->next;
		}
		
		return index;
	}
	else {
		return -1;
	}
}

MenuItem::operator GtkWidget* () {
	// Check for toggle, allocate the GtkWidget*
	if (!_constructed) {
		construct();
	}
	
	return _widget;
}

MenuItemPtr MenuItem::find(const std::string& menuPath) {
	// Split the path and analyse it
	StringVector parts;
	boost::algorithm::split(parts, menuPath, boost::algorithm::is_any_of("/"));
	
	// Any path items at all?
	if (parts.size() > 0) {
		MenuItemPtr child;
		
		// Path is not empty, try to find the first item among the item's children
		for (unsigned int i = 0; i < _children.size(); i++) {
			if (_children[i]->getName() == parts[0]) {
				child = _children[i];
			}
		}
		
		// The topmost name seems to be part of the children, pass the call
		if (child != NULL) {
			// Is this the end of the path (no more items)?
			if (parts.size() == 1) {
				// Yes, return the found item
				return child;
			}
			else {
				// No, pass the query down the hierarchy
				std::string childPath("");
				for (unsigned int i = 1; i < parts.size(); i++) {
					childPath += (childPath != "") ? "/" : "";
					childPath += parts[i];
				}
				return child->find(childPath);
			}
		}
	}
	
	// Nothing found, return NULL pointer
	return MenuItemPtr();
}

void MenuItem::parseNode(xml::Node& node, MenuItemPtr thisItem) {
	std::string nodeName = node.getName();
	
	setName(node.getAttributeValue("name"));
	setCaption(node.getAttributeValue("caption"));
	
	if (nodeName == "menuItem") {
		_type = menuItem;
		// Get the EventPtr according to the event
		setEvent(node.getAttributeValue("command"));
		setIcon(node.getAttributeValue("icon"));
	}
	else if (nodeName == "menuSeparator") {
		_type = menuSeparator;
	}
	else if (nodeName == "subMenu") {
		_type = menuFolder;
		
		xml::NodeList subNodes = node.getChildren();
		for (unsigned int i = 0; i < subNodes.size(); i++) {
			if (subNodes[i].getName() != "text") {
				// Allocate a new child menuitem with a pointer to <self>
				MenuItemPtr newChild = MenuItemPtr(new MenuItem(thisItem));
				// Let the child parse the subnode
				newChild->parseNode(subNodes[i], newChild);
				
				// Add the child to the list
				_children.push_back(newChild);
			}
		}
	}
	else if (nodeName == "menu") {
		_type = menuBar;
		
		xml::NodeList subNodes = node.getChildren();
		for (unsigned int i = 0; i < subNodes.size(); i++) {
			if (subNodes[i].getName() != "text") {
				// Allocate a new child menuitem with a pointer to <self>
				MenuItemPtr newChild = MenuItemPtr(new MenuItem(thisItem));
				// Let the child parse the subnode
				newChild->parseNode(subNodes[i], newChild);
				
				// Add the child to the list
				_children.push_back(newChild);
			}
		}
	} 
	else if (nodeName == "menuMRU") {
		_type = menuMRU;
	}
	else {
		_type = menuNothing;
		globalErrorStream() << "MenuItem: Unknown node found: " << nodeName.c_str() << "\n"; 
	}
}

void MenuItem::construct() {
	if (_type == menuBar) {
		_widget = gtk_menu_bar_new();
		for (unsigned int i = 0; i < _children.size(); i++) {
			// Cast each children onto GtkWidget and append it to the menu
			gtk_menu_shell_append(GTK_MENU_SHELL(_widget), *_children[i]);
		}
	}
	else if (_type == menuSeparator) {
		_widget = gtk_separator_menu_item_new();
	}
	else if (_type == menuFolder) {
		// Create the menuitem
		_widget = gtk_menu_item_new_with_mnemonic(_caption.c_str());
		// Create the submenu
		GtkWidget* subMenu = gtk_menu_new();
		gtk_widget_show(subMenu);
		// Attach the submenu to the menuitem
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(_widget), subMenu);
		
		for (unsigned int i = 0; i < _children.size(); i++) {
			
			// Special case: the MRU menu (not a folder, but a series of widgets)
			if (_children[i]->getType() == menuMRU) {
				WidgetList widgetList = GlobalMRU().getMenuWidgets();
		
				for (unsigned int w = 0; w < widgetList.size(); w++) {
					gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), widgetList[w]);
				}
			}
			else {
				// Cast each children onto GtkWidget and append it to the menu
				gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), *_children[i]);
			}
		}
	}
	else if (_type == menuItem) {
		// Try to lookup the event name
		IEventPtr event = GlobalEventManager().findEvent(_event);
		
		if (!event->empty()) {
			// Retrieve an acclerator string formatted for a menu
			const std::string accelText = 
				GlobalEventManager().getAcceleratorStr(event, true);
			 
			// Create a new menuitem
			_widget = gtkutil::TextMenuItemAccelerator(_caption,
														accelText, 
														_icon,
														event->isToggle());

			gtk_widget_show_all(_widget);
			// Connect the widget to the event
			event->connectWidget(_widget);
		}
		else {
			globalErrorStream() << "MenuItem: Cannot find associated event: " << _event.c_str() << "\n"; 
		}
	}
	else if (_type == menuMRU) {
		// Do nothing.
	}
	else if (_type == menuRoot) {
		globalErrorStream() << "MenuItem: Cannot instantiate root MenuItem.\n"; 
	}
	
	if (_widget != NULL) {
		gtk_widget_show(_widget);
	}
	
	_constructed = true;
}

} // namespace ui
