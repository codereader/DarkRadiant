#include "MenuItem.h"

#include "i18n.h"
#include "itextstream.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

#include <wx/menu.h>
#include <wx/menuitem.h>

#include <iostream>

#include "LocalBitmapArtProvider.h"

namespace ui
{

int MenuItem::_nextMenuItemId = 100;

MenuItem::MenuItem(const MenuItemPtr& parent) :
	_parent(parent ? MenuItemWeakPtr(parent) : MenuItemWeakPtr()),
	_widget(nullptr),
	_type(menuNothing),
	_constructed(false)
{}

MenuItem::~MenuItem()
{
	disconnectEvent();
}

std::string MenuItem::getName() const
{
	return _name;
}

void MenuItem::setName(const std::string& name) 
{
	_name = name;
}

bool MenuItem::isRoot() const
{
	return (_type == menuRoot);
}

MenuItemPtr MenuItem::parent() const
{
	return _parent.lock();
}

void MenuItem::setParent(const MenuItemPtr& parent)
{
	_parent = parent;
}

void MenuItem::setCaption(const std::string& caption) 
{
	_caption = caption;
}

std::string MenuItem::getCaption() const
{
	return _caption;
}

void MenuItem::setIcon(const std::string& icon)
{
	_icon = icon;
}

bool MenuItem::isEmpty() const 
{
	return (_type != menuItem);
}

eMenuItemType MenuItem::getType() const
{
	return _type;
}

void MenuItem::setType(eMenuItemType type)
{
	_type = type;
}

std::size_t MenuItem::numChildren() const 
{
	return _children.size();
}

void MenuItem::addChild(const MenuItemPtr& newChild)
{
	newChild->setParent(shared_from_this());
	_children.push_back(newChild);
}

void MenuItem::removeChild(const MenuItemPtr& child)
{
	for (MenuItemList::iterator i = _children.begin(); i != _children.end(); ++i)
	{
		if (*i == child)
		{
			child->setParent(MenuItemPtr());
			_children.erase(i);
			return;
		}
	}
}

void MenuItem::removeAllChildren()
{
	for (const MenuItemPtr& child : _children)
	{
		child->setParent(MenuItemPtr());
	}

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
	std::list<std::string> parts;
	boost::algorithm::split(parts, menuPath, boost::algorithm::is_any_of("/"));

	// Any path items at all?
	if (parts.empty()) return MenuItemPtr();

	// Path is not empty, try to find the first item among the item's children
	for (const MenuItemPtr& candidate : _children)
	{
		if (candidate->getName() == parts.front())
		{
			// Remove the first part, it has been processed
			parts.pop_front();

			// Is this the end of the path (no more items)?
			if (parts.empty()) 
			{
				// Yes, return the found item
				return candidate;
			}
			
			// No, pass the query down the hierarchy
			std::string childPath = boost::algorithm::join(parts, "/");

			return candidate->find(childPath);
		}
	}

	// Nothing found
	return MenuItemPtr();
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

eMenuItemType MenuItem::GetTypeForXmlNode(const xml::Node& node)
{
	std::string nodeName = node.getName();

	if (nodeName == "menuItem")
	{
		return menuItem;
	}
	else if (nodeName == "menuSeparator")
	{
		return menuSeparator;
	}
	else if (nodeName == "subMenu")
	{
		return menuFolder;
	}
	else if (nodeName == "menu")
	{
		return menuBar;
	}
	else
	{
		rError() << "MenuItem: Unknown node found: " << node.getName() << std::endl;
		return menuNothing;
	}
}

MenuItemPtr MenuItem::CreateFromNode(const xml::Node& node)
{
	MenuItemPtr item = std::make_shared<MenuItem>();

	item->setName(node.getAttributeValue("name"));

	// Put the caption through gettext before passing it to setCaption
	item->setCaption(_(node.getAttributeValue("caption").c_str()));

	// Parse type name
	item->setType(GetTypeForXmlNode(node));

	if (item->getType() == menuItem)
	{
		// Get the EventPtr according to the event
		item->setEvent(node.getAttributeValue("command"));
		item->setIcon(node.getAttributeValue("icon"));
	}
	else if (item->getType() == menuNothing)
	{
		return MenuItemPtr();
	}

	// Parse subnodes
	xml::NodeList childNodes = node.getChildren();

	for (const xml::Node& childNode : childNodes)
	{
		if (childNode.getName() == "text" || childNode.getName() == "comment")
		{
			continue;
		}
		
		// Allocate a new child item
		MenuItemPtr childItem = CreateFromNode(childNode);

		// Add the child to the list
		if (childItem)
		{
			item->addChild(childItem);
		}
	}

	return item;
}

} // namespace ui
