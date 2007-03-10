#include "MenuItem.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <iostream>

namespace ui {
	
	namespace {
		typedef std::vector<std::string> StringVector;
	}

MenuItem::MenuItem(MenuItemPtr parent) :
	_parent(parent),
	_widget(NULL),
	_type(eNothing)
{
	if (_parent == NULL) {
		_type = eRoot;
	}
}

std::string MenuItem::getName() const {
	return _name;
}

void MenuItem::setName(const std::string& name) {
	_name = name;
}

bool MenuItem::isRoot() const {
	return (_type == eRoot);
}

MenuItemPtr MenuItem::getParent() const {
	return _parent;
}
	
void MenuItem::setCaption(const std::string& caption) {
	_caption = caption;
}

std::string MenuItem::getCaption() const {
	return _caption;
}

bool MenuItem::isEmpty() const {
	return (_event == NULL || _event->empty());
}

MenuItem::eType MenuItem::getType() const {
	return _type;
}

int MenuItem::numChildren() const {
	return _children.size();
}

IEventPtr MenuItem::getEvent() const {
	return _event;
}

void MenuItem::setEvent(const std::string& eventName) {
	_event = GlobalEventManager().findEvent(eventName);
}

MenuItem::operator GtkWidget* () {
	// Check for toggle, allocate the GtkWidget*
	return NULL;
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
		_type = eItem;
		// Get the EventPtr according to the event
		setEvent(node.getAttributeValue("event"));
	}
	else if (nodeName == "menuSeparator") {
		_type = eSeparator;
	}
	else if (nodeName == "subMenu") {
		_type = eFolder;
		
		xml::NodeList subNodes = node.getChildren();
		for (unsigned int i = 0; i < subNodes.size(); i++) {
			if (subNodes[i].getName() != "text") {
				// Allocate a new child menuitem with a pointer to <self>
				MenuItemPtr newChild = MenuItemPtr(new MenuItem(thisItem));
				// Let the child parse the subnode
				newChild->parseNode(subNodes[i], newChild);
			}
		}
	}
	else if (nodeName == "menu") {
		_type = eRoot;
		
		xml::NodeList subNodes = node.getChildren();
		for (unsigned int i = 0; i < subNodes.size(); i++) {
			if (subNodes[i].getName() != "text") {
				// Allocate a new child menuitem with a pointer to <self>
				MenuItemPtr newChild = MenuItemPtr(new MenuItem(thisItem));
				// Let the child parse the subnode
				newChild->parseNode(subNodes[i], newChild);
			}
		}
	} 
	else {
		_type = eNothing;
		globalErrorStream() << "MenuItem: Unknown node found: " << nodeName.c_str() << "\n"; 
	}
}

} // namespace ui
