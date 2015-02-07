#include "MenuItem.h"

#include "i18n.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include <wx/menu.h>
#include <wx/menuitem.h>

#include <iostream>

#include "LocalBitmapArtProvider.h"

namespace ui
{

namespace 
{
	typedef std::vector<std::string> StringVector;
}

int MenuItem::_nextMenuItemId = 100;

MenuItem::MenuItem(const MenuItemPtr& parent) :
	_parent(MenuItemWeakPtr(parent)),
	_widget(NULL),
	_type(menuNothing),
	_constructed(false)
{
	if (parent == NULL) {
		_type = menuRoot;
	}
	else if (parent->isRoot()) {
		_type = menuBar;
	}
}

MenuItem::~MenuItem()
{
	disconnectEvent();
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
	return _parent.lock();
}

void MenuItem::setParent(const MenuItemPtr& parent) {
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

std::size_t MenuItem::numChildren() const {
	return _children.size();
}

void MenuItem::addChild(const MenuItemPtr& newChild) {
	_children.push_back(newChild);
}

void MenuItem::removeChild(const MenuItemPtr& child)
{
	for (MenuItemList::iterator i = _children.begin(); i != _children.end(); ++i)
	{
		if (*i == child)
		{
			_children.erase(i);
			return;
		}
	}
}

void MenuItem::removeAllChildren()
{
	_children.clear();
}

std::string MenuItem::getEvent() const
{
	return _event;
}

void MenuItem::setEvent(const std::string& eventName)
{
	_event = eventName;
}

int MenuItem::getMenuPosition(const MenuItemPtr& child)
{
	if (!_constructed)
	{
		construct();
	}

	// Check if this is the right item type for this operation
	if (_type == menuFolder)
	{
		// A menufolder is a menuitem with a contained submenu, retrieve it
		wxMenu* container = dynamic_cast<wxMenu*>(_widget);

		// Get the list of child widgets
		wxMenuItemList& children = container->GetMenuItems();

		// The child Widget for comparison
		wxObject* childWidget = child->getWidget();

		int position = 0;
		for (wxMenuItemList::const_iterator i = children.begin(); i != children.end(); ++i, ++position)
		{
			// Get the widget pointer from the current list item
			wxMenuItem* item = *i;

			if (item == childWidget || 
				(child->getType() == menuFolder && item->GetSubMenu() == childWidget))
			{
				return position;
			}
		}
	}
	else if (_type == menuBar)
	{
		wxMenuBar* container = dynamic_cast<wxMenuBar*>(_widget);
		
		if (container == NULL)
		{
			rWarning() << "Cannot find menu position, cannot cast to wxMenuBar." << std::endl;
			return -1;
		}

		// The child Widget for comparison
		wxObject* childWidget = child->getWidget();

		// Iterate over all registered menus
		for (int position = 0; position < container->GetMenuCount(); ++position)
		{
			// Get the widget pointer from the current list item
			if (container->GetMenu(position) == childWidget)
			{
				return position;
			}
		}
	}

	return -1; // not found or wrong type
}

wxObject* MenuItem::getWidget()
{
	// Check for toggle, allocate the Gtk::Widget*
	if (!_constructed)
	{
		construct();
	}

	return _widget;
}

MenuItemPtr MenuItem::find(const std::string& menuPath)
{
	// Split the path and analyse it
	StringVector parts;
	boost::algorithm::split(parts, menuPath, boost::algorithm::is_any_of("/"));

	// Any path items at all?
	if (!parts.empty()) {
		MenuItemPtr child;

		// Path is not empty, try to find the first item among the item's children
		for (std::size_t i = 0; i < _children.size(); i++) {
			if (_children[i]->getName() == parts[0]) {
				child = _children[i];
				break;
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
				for (std::size_t i = 1; i < parts.size(); i++) {
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

void MenuItem::parseNode(xml::Node& node, const MenuItemPtr& thisItem) {
	std::string nodeName = node.getName();

	setName(node.getAttributeValue("name"));

	// Put the caption through gettext before passing it to setCaption
	setCaption(_(node.getAttributeValue("caption").c_str()));

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
		for (std::size_t i = 0; i < subNodes.size(); i++) {
			if (subNodes[i].getName() != "text" && subNodes[i].getName() != "comment") {
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
		for (std::size_t i = 0; i < subNodes.size(); i++) {
			if (subNodes[i].getName() != "text" && subNodes[i].getName() != "comment") {
				// Allocate a new child menuitem with a pointer to <self>
				MenuItemPtr newChild = MenuItemPtr(new MenuItem(thisItem));
				// Let the child parse the subnode
				newChild->parseNode(subNodes[i], newChild);

				// Add the child to the list
				_children.push_back(newChild);
			}
		}
	}
	else {
		_type = menuNothing;
		rError() << "MenuItem: Unknown node found: " << nodeName << std::endl;
	}
}

void MenuItem::construct()
{
	if (_type == menuBar)
	{
		wxMenuBar* menuBar = new wxMenuBar;
		_widget = menuBar;

		for (std::size_t i = 0; i < _children.size(); i++)
		{
			// Cast each children onto wxMenu and append it to the menu
			wxMenu* menu = dynamic_cast<wxMenu*>(_children[i]->getWidget());

			if (menuItem != NULL)
			{
				menuBar->Append(menu, _children[i]->getCaption());
			}
			else
			{
				rError() << "MenuItem::construct: Cannot cast child to wxMenu" << std::endl;
			}
		}
	}
	else if (_type == menuSeparator)
	{
		_widget = NULL; // separator is handled when adding to the parent menu itself
	}
	else if (_type == menuFolder)
	{
		// Create the menuitem, don't pass a title to the constructor,
		// otherwise the caption gets immediately added as first child
		wxMenu* menu = new wxMenu;
		_widget = menu;

		for (std::size_t i = 0; i < _children.size(); i++)
		{
			if (_children[i]->getType() == menuSeparator)
			{
				_children[i]->_widget = menu->AppendSeparator();
				continue;
			}

			wxMenuItem* menuItem = dynamic_cast<wxMenuItem*>(_children[i]->getWidget());

			if (menuItem != NULL)
			{
				menu->Append(menuItem);

				if (_children[i]->getEvent().empty())
				{
					// No event attached to this menu item, disable it
					menu->Enable(menuItem->GetId(), false);
					continue;
				}
				
				// Now is the time to connect the event, the item has  a valid menu parent at this point
				IEventPtr event = GlobalEventManager().findEvent(_children[i]->getEvent());

				if (event != NULL)
				{
					event->connectMenuItem(menuItem);
				}

				continue;
			}
			
			wxMenu* subMenu = dynamic_cast<wxMenu*>(_children[i]->getWidget());

			if (subMenu != NULL)
			{
				menu->AppendSubMenu(subMenu, _children[i]->getCaption());
				continue;
			}
		}
	}
	else if (_type == menuItem)
	{
		if (!_event.empty())
		{
			// Try to lookup the event name
			IEventPtr event = GlobalEventManager().findEvent(_event);

			if (!event->empty())
			{
				// Retrieve an accelerator string formatted for a menu
				const std::string accelText =
					GlobalEventManager().getAcceleratorStr(event, true);

				// Create a new menuitem
				// greebo: Accelerators seem to globally catch the key events, add a space to fool wxWidgets
				wxMenuItem* item = new wxMenuItem(NULL, _nextMenuItemId++, _caption + "\t " + accelText);
				_widget = item;

				if (!_icon.empty())
				{
					item->SetBitmap(wxArtProvider::GetBitmap(LocalBitmapArtProvider::ArtIdPrefix() + _icon));
				}

				item->SetCheckable(event->isToggle());
			}
			else
			{
				rWarning() << "MenuItem: Cannot find associated event: " << _event << std::endl;
			}
		}
		else
		{
			// Create an empty, desensitised menuitem, will be disabled once attached to the parent menu
			wxMenuItem* item = new wxMenuItem(NULL, _nextMenuItemId++, _caption.empty() ? "-" : _caption);

			_widget = item;
		}
	}
	else if (_type == menuRoot)
	{
		// Cannot instantiate root MenuItem, ignore
	}

	_constructed = true;
}

void MenuItem::connectEvent()
{
	if (!_event.empty() && _type == menuItem)
	{
		// Try to lookup the event name
		IEventPtr event = GlobalEventManager().findEvent(_event);
		wxMenuItem* menuItem = dynamic_cast<wxMenuItem*>(_widget);

		if (event != NULL && menuItem != NULL)
		{
			event->connectMenuItem(menuItem);
		}
	}
}

void MenuItem::disconnectEvent()
{
	if (!_event.empty() && _type == menuItem)
	{
		IEventPtr ev = GlobalEventManager().findEvent(_event);
		wxMenuItem* item = dynamic_cast<wxMenuItem*>(_widget);

		// Tell the eventmanager to disconnect the widget in any case
		// even if has been destroyed already.
		if (ev != NULL && item != NULL)
		{
			ev->disconnectMenuItem(item);
		}
	}
}

void MenuItem::setWidget(wxObject* object)
{
	// Disconnect the old widget before setting a new one
	disconnectEvent();

	_widget = object;
	_constructed = true;
}

} // namespace ui
