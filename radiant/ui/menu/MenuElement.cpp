#include "MenuElement.h"

#include "i18n.h"
#include "itextstream.h"
#include "igame.h"

#include "string/split.h"
#include "string/join.h"

#include <wx/menu.h>
#include <wx/menuitem.h>

#include <limits>
#include <iterator>
#include <iostream>

#include "MenuBar.h"
#include "MenuFolder.h"
#include "MenuItem.h"
#include "MenuSeparator.h"

namespace ui
{

namespace menu
{

int MenuElement::_nextMenuItemId = 100;

MenuElement::MenuElement(const MenuElementPtr& parent) :
	_parent(parent ? MenuElementWeakPtr(parent) : MenuElementWeakPtr()),
	_isVisible(true),
	_needsRefresh(false)
{}

MenuElement::~MenuElement()
{}

const std::string& MenuElement::getName() const
{
	return _name;
}

void MenuElement::setName(const std::string& name) 
{
	_name = name;
}

MenuElementPtr MenuElement::getParent() const
{
	return _parent.lock();
}

void MenuElement::setParent(const MenuElementPtr& parent)
{
	_parent = parent;
}

void MenuElement::setCaption(const std::string& caption) 
{
	_caption = caption;
}

std::string MenuElement::getCaption() const
{
	return _caption;
}

void MenuElement::setIcon(const std::string& icon)
{
	_icon = icon;
}

std::size_t MenuElement::numChildren() const 
{
	return _children.size();
}

bool MenuElement::isVisible() const
{
	return _isVisible;
}

void MenuElement::setIsVisible(bool visible)
{
	_isVisible = visible;
}

void MenuElement::addChild(const MenuElementPtr& newChild)
{
	newChild->setParent(shared_from_this());
	_children.push_back(newChild);
}

void MenuElement::insertChild(const MenuElementPtr& newChild, const MenuElementPtr& insertBefore)
{
	newChild->setParent(shared_from_this());
	_children.insert(std::find(_children.begin(), _children.end(), insertBefore), newChild);
}

void MenuElement::removeChild(const MenuElementPtr& child)
{
	for (MenuElementList::iterator i = _children.begin(); i != _children.end(); ++i)
	{
		if (*i == child)
		{
			// Deconstruct the child before removal
			child->deconstruct();

			// Release from parent and remove from the list
			child->setParent(MenuElementPtr());
			_children.erase(i);
			break;
		}
	}
}

void MenuElement::removeAllChildren()
{
	for (const MenuElementPtr& child : _children)
	{
		child->setParent(MenuElementPtr());
	}

	_children.clear();
}

const std::string& MenuElement::getEvent() const
{
	return _event;
}

void MenuElement::setEvent(const std::string& eventName)
{
	_event = eventName;
}

int MenuElement::getMenuPosition(const MenuElementPtr& child, bool includeHidden)
{
	int pos = 0;

	for (const MenuElementPtr& candidate : _children)
	{
		if (!candidate->isVisible()) continue; // skip hidden items
		
		if (candidate == child) break;
		
		++pos;
	}

	return pos;
}

MenuElementPtr MenuElement::find(const std::string& menuPath)
{
	// Split the path and analyse it
	std::list<std::string> parts;
	string::split(parts, menuPath, "/");

	// Any path items at all?
	if (parts.empty()) return MenuElementPtr();

	// Path is not empty, try to find the first item among the item's children
	for (const MenuElementPtr& candidate : _children)
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
			std::string childPath = string::join(parts, "/");

			return candidate->find(childPath);
		}
	}

	// Nothing found
	return MenuElementPtr();
}

bool MenuElement::needsRefresh()
{
	return _needsRefresh;
}

void MenuElement::setNeedsRefresh(bool needsRefresh)
{
	_needsRefresh = needsRefresh;
}

void MenuElement::setAccelerator(const std::string& accelStr)
{}

MenuElementPtr MenuElement::CreateFromNode(const xml::Node& node)
{
    MenuElementPtr item;

    // Don't create anything if the menu item requires a non-existent game feature
    if (auto feature = node.getAttributeValue("gamefeature");
        !feature.empty() && !GlobalGameManager().currentGame()->hasFeature(feature))
    {
        return {};
    }

    std::string nodeName = node.getName();

    if (nodeName == "menuItem")
    {
        item = std::make_shared<MenuItem>();

        // Get the EventPtr according to the event
        item->setEvent(node.getAttributeValue("command"));
        item->setIcon(node.getAttributeValue("icon"));
    }
    else if (nodeName == "menuSeparator")
    {
        item = std::make_shared<MenuSeparator>();
    }
    else if (nodeName == "subMenu")
    {
        item = std::make_shared<MenuFolder>();
    }
    else if (nodeName == "menu")
    {
        item = std::make_shared<MenuBar>();
    }
    else
    {
        rError() << "MenuElement: Unknown node found: " << node.getName() << std::endl;
        return item;
    }

    item->setName(node.getAttributeValue("name"));

    // Put the caption through gettext before passing it to setCaption
    item->setCaption(_(node.getAttributeValue("caption").c_str()));

    // Parse subnodes
    xml::NodeList childNodes = node.getChildren();

    for (const xml::Node& childNode : childNodes)
    {
        if (childNode.getName() == "text" || childNode.getName() == "comment")
        {
            continue;
        }
        
        // Allocate a new child item
        MenuElementPtr childItem = CreateFromNode(childNode);

        // Add the child to the list
        if (childItem)
            item->addChild(childItem);
    }

    return item;
}

MenuElementPtr MenuElement::CreateForType(ItemType type)
{
	MenuElementPtr item;

	switch (type)
	{
    case ItemType::Item:
		item = std::make_shared<MenuItem>();
		break;
	case ItemType::Bar:
		item = std::make_shared<MenuBar>();
		break;
	case ItemType::Folder:
		item = std::make_shared<MenuFolder>();
		break;
	case ItemType::Separator:
		item = std::make_shared<MenuSeparator>();
		break;
	default:
		rError() << "MenuElement: Cannot create node for type " << (int)type << std::endl;
	};

	return item;
}

void MenuElement::setNeedsRefreshRecursively(bool needsRefresh)
{
	setNeedsRefresh(needsRefresh);

	for (const MenuElementPtr& child : _children)
	{
		child->setNeedsRefreshRecursively(needsRefresh);
	}
}

void MenuElement::constructChildren()
{
	for (const MenuElementPtr& child : _children)
	{
		child->construct();
	}
}

void MenuElement::deconstructChildren()
{
	for (const MenuElementPtr& child : _children)
	{
		child->deconstruct();
	}
}

} // namespace

} // namespace
