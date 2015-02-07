#include "MenuManager.h"

#include "itextstream.h"
#include "iregistry.h"

#include <wx/menu.h>
#include <wx/menuitem.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

namespace ui 
{

namespace 
{
	// The menu root key in the registry
	const std::string RKEY_MENU_ROOT = "user/ui/menu";
	const std::string TYPE_ITEM = "item";
	typedef std::vector<std::string> StringVector;
}

MenuManager::MenuManager() :
	_root(new MenuItem(MenuItemPtr())) // Allocate the root item (type is set automatically)
{}

void MenuManager::clear() {
	_root = MenuItemPtr();
}

void MenuManager::loadFromRegistry() {
	xml::NodeList menuNodes = GlobalRegistry().findXPath(RKEY_MENU_ROOT);

	if (!menuNodes.empty()) {
		for (std::size_t i = 0; i < menuNodes.size(); i++) {
			std::string name = menuNodes[i].getAttributeValue("name");

			// Allocate a new MenuItem with root as parent
			MenuItemPtr menubar = MenuItemPtr(new MenuItem(_root));
			menubar->setName(name);

			// Populate the root menuitem using the current node
			menubar->parseNode(menuNodes[i], menubar);

			// Add the menubar as child of the root (child is already parented to _root)
			_root->addChild(menubar);
		}
	}
	else {
		rError() << "MenuManager: Could not find menu root in registry.\n";
	}
}

void MenuManager::setVisibility(const std::string& path, bool visible)
{
	// Sanity check for empty menu
	if (_root == NULL) return;

	MenuItemPtr foundMenu = _root->find(path);

	if (foundMenu != NULL)
	{
		// Get the Widget* and set the visibility
		wxMenuItem* menuitem = dynamic_cast<wxMenuItem*>(foundMenu->getWidget());

		if (menuitem == NULL)
		{
			return;
		}

		if (visible)
		{
			menuitem->Enable(true);
		}
		else {
			menuitem->Enable(false);
		}
	}
	else {
		rError() << "MenuManager: Warning: Menu " << path << " not found!\n";
	}
}

wxObject* MenuManager::get(const std::string& path)
{
	// Sanity check for empty menu
	if (_root == NULL) return NULL;

	MenuItemPtr foundMenu = _root->find(path);

	if (foundMenu != NULL)
	{
		return foundMenu->getWidget();
	}
	else
	{
		//rError() << "MenuManager: Warning: Menu " << path.c_str() << " not found!\n";
		return NULL;
	}
}

wxObject* MenuManager::add(const std::string& insertPath,
							const std::string& name,
							eMenuItemType type,
							const std::string& caption,
							const std::string& icon,
							const std::string& eventName)
{
	// Sanity check for empty menu
	if (_root == NULL) return NULL;

	MenuItemPtr found = _root->find(insertPath);

	if (found != NULL)
	{
		// Allocate a new MenuItem
		MenuItemPtr newItem = MenuItemPtr(new MenuItem(found));

		newItem->setName(name);
		newItem->setCaption(caption);
		newItem->setType(type);
		newItem->setIcon(icon);
		newItem->setEvent(eventName);

		// Get the parent widget
		wxObject* parentItem = found->getWidget();

		if (found->getType() == menuBar)
		{
			// The parent is a menubar, require a menu in the first place
			if (newItem->getType() != menuFolder)
			{
				rError() << "Cannot insert non-menu into menu bar." << std::endl;
				return NULL;
			}

			wxMenu* newMenu = static_cast<wxMenu*>(newItem->getWidget());
			static_cast<wxMenuBar*>(parentItem)->Append(newMenu, newItem->getCaption());
		}
		else
		{
			// Retrieve the submenu widget from the item
			wxMenu* menu = dynamic_cast<wxMenu*>(parentItem);

			if (menu == NULL)
			{
				rError() << "Cannot cast parent item to a wxMenu." << std::endl;
				return NULL;
			}

			// Special handling for separators
			if (newItem->getType() == menuSeparator)
			{
				newItem->setWidget(menu->AppendSeparator());
			}

			wxMenuItem* item = dynamic_cast<wxMenuItem*>(newItem->getWidget());

			if (item != NULL && newItem->getType() != menuSeparator)
			{
				menu->Append(item);

				if (!newItem->getEvent().empty())
				{
					newItem->connectEvent();
				}
				else
				{
					// No event, disable this item
					menu->Enable(item->GetId(), false);
				}
			}
		}

		// Add the child to the <found> parent, AFTER its wxMenuItem* operator
		// was invoked, otherwise the parent tries to instantiate it before it's actually
		// added.
		found->addChild(newItem);

		return newItem->getWidget();
	}
	else if (insertPath.empty())
	{
		// We have a new top-level menu item, create it as child of root
		MenuItemPtr newItem = MenuItemPtr(new MenuItem(_root));

		newItem->setName(name);
		newItem->setCaption(caption);
		newItem->setType(type);
		newItem->setIcon(icon);
		newItem->setEvent(eventName);

		// Insert into root
		_root->addChild(newItem);

		return newItem->getWidget();
	}
	else {
		// not found and not a top-level item either.
	}

	return NULL;
}

wxObject* MenuManager::insert(const std::string& insertPath,
						 const std::string& name,
						 eMenuItemType type,
						 const std::string& caption,
						 const std::string& icon,
						 const std::string& eventName)
{
	// Sanity check for empty menu
	if (_root == NULL) return NULL;

	MenuItemPtr found = _root->find(insertPath);

	if (found != NULL)
	{
		if (found->parent() != NULL)
		{
			// Get the Menu position of the child widget
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

			wxObject* parentWidget = found->parent()->getWidget();

			// Insert it at the given position
			if (found->parent()->getType() == menuBar)
			{
				// The parent is a menubar, require a menu in the first place
				if (newItem->getType() != menuFolder)
				{
					rError() << "Cannot insert non-menu into menu bar." << std::endl;
					return NULL;
				}

				wxMenu* newMenu = static_cast<wxMenu*>(newItem->getWidget());
				static_cast<wxMenuBar*>(parentWidget)->Insert(position, newMenu, newItem->getCaption());
			}
			else if (found->parent()->getType() == menuFolder)
			{
				// The parent is a submenu (=menuitem), try to retrieve the menushell first
				wxMenu* menu = dynamic_cast<wxMenu*>(parentWidget);

				if (newItem->getType() == menuSeparator)
				{
					newItem->setWidget(menu->InsertSeparator(position));
					return newItem->getWidget();
				}
				else if (newItem->getType() == menuFolder)
				{
					wxMenu* subMenu = dynamic_cast<wxMenu*>(newItem->getWidget());

					if (subMenu == NULL)
					{
						rError() << "Cannot cast item to a wxMenu." << std::endl;
						return NULL;
					}

					if (menu != NULL)
					{
						menu->Insert(position, wxID_ANY, newItem->getCaption(), subMenu);
					}
					else
					{
						rError() << "Cannot cast parent item to a wxMenu*." << std::endl;
					}
				}
				else if (newItem->getType() == menuItem)
				{
					wxMenuItem* item = dynamic_cast<wxMenuItem*>(newItem->getWidget());

					if (item == NULL)
					{
						rError() << "Cannot cast item to a wxMenuItem." << std::endl;
						return NULL;
					}

					if (menu != NULL)
					{
						menu->Insert(position, item);

						if (!newItem->getEvent().empty())
						{
							newItem->connectEvent();
						}
						else
						{
							// No event, disable this item
							menu->Enable(item->GetId(), false);
						}
					}
					else
					{
						rError() << "Cannot cast parent item to a wxMenu*." << std::endl;
					}
				}
			}

			return newItem->getWidget();
		}
		else
		{
			rError() << "MenuManager: Unparented menuitem, can't determine position: ";
			rError() << insertPath << std::endl;
			return NULL;
		}
	}
	else
	{
		rError() << "MenuManager: Could not find insertPath: " << insertPath << std::endl;
		return NULL;
	}
}

void MenuManager::remove(const std::string& path)
{
	// Sanity check for empty menu
	if (_root == NULL) return;

	MenuItemPtr item = _root->find(path);

	if (item == NULL) return; // nothing to do

	MenuItemPtr parent = item->parent();

	if (parent == NULL) return; // no parent ?

	if (parent->getType() == menuFolder)
	{
		wxMenu* parentMenu = static_cast<wxMenu*>(parent->getWidget());

        if (item->getType() == menuItem)
        {
            wxMenuItem* wxItem = static_cast<wxMenuItem*>(item->getWidget());

            // Disconnect the item safely before going ahead
            item->setWidget(NULL);

            // This will delete the item
            parentMenu->Destroy(wxItem);
        }
        else if (item->getType() == menuFolder)
        {
            wxMenu* wxSubmenu = static_cast<wxMenu*>(item->getWidget());

            // Disconnect the item safely before going ahead
            item->removeAllChildren();
            item->setWidget(NULL);

            // Get the list of child widgets
            wxMenuItemList& children = parentMenu->GetMenuItems();

            for (wxMenuItemList::const_iterator i = children.begin(); i != children.end(); ++i)
            {
                // Get the widget pointer from the current list item
                wxMenuItem* candidate = *i;

                if (candidate->GetSubMenu() == wxSubmenu)
                {
                    parentMenu->Destroy(candidate);
                    break;
                }
            }
        }
	}
	else if (parent->getType() == menuBar)
	{
		wxMenuBar* parentBar = static_cast<wxMenuBar*>(parent->getWidget());
		wxMenu* menu = static_cast<wxMenu*>(item->getWidget());

		int oldPosition = parent->getMenuPosition(item);

		if (oldPosition != -1)
		{
			// Disconnect the item safely before going ahead
			item->removeAllChildren();
			item->setWidget(NULL);

			menu = parentBar->Remove(oldPosition);
			delete menu;
		}
		else
		{
			rWarning() << "Cannot remove menu from menu bar, menu position not found." << std::endl;
		}
	}

	// Remove the found item from the parent menu item
	parent->removeChild(item);
}

} // namespace ui
