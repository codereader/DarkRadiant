#include "MenuManager.h"

#include "itextstream.h"
#include "iregistry.h"

#include <wx/menu.h>
#include <wx/menuitem.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

#include "MenuBar.h"
#include "MenuFolder.h"
#include "MenuRootElement.h"

namespace ui 
{

namespace 
{
	// The menu root key in the registry
	const char* const RKEY_MENU_ROOT = "user/ui/menu";
}

MenuManager::MenuManager() :
	_root(new MenuRootElement())
{}

void MenuManager::clear()
{
	_root.reset();
}

void MenuManager::loadFromRegistry()
{
	// Clear existing elements first
	_root.reset(new MenuRootElement());

	xml::NodeList menuNodes = GlobalRegistry().findXPath(RKEY_MENU_ROOT);

	if (!menuNodes.empty())
	{
		for (const xml::Node& menuNode : menuNodes)
		{
			MenuElementPtr menubar = MenuElement::CreateFromNode(menuNode);
			
			// Add the menubar as child of the root
			_root->addChild(menubar);
		}
	}
	else 
	{
		rError() << "MenuManager: Could not find menu root in registry." << std::endl;
	}
}

void MenuManager::setVisibility(const std::string& path, bool visible)
{
	// Sanity check for empty menu
	if (!_root) return;

	MenuElementPtr element = _root->find(path);

	if (element)
	{
		element->setIsVisible(visible);

		// The corresponding menu should be reconstructed
		MenuElementPtr parentMenu = findParentMenu(element);

		if (parentMenu)
		{
			//parentMenu->deconstruct();
		}
	}
}

wxMenuBar* MenuManager::getMenuBar(const std::string& name)
{
	MenuElementPtr menuBar = _root->find(name);
	
	if (menuBar)
	{
		assert(std::dynamic_pointer_cast<MenuBar>(menuBar));

		return std::static_pointer_cast<MenuBar>(menuBar)->getWidget();
	}
	
	rError() << "MenuManager: Warning: Menubar with name " << name << " not found!" << std::endl;
	return nullptr;
}

wxObject* MenuManager::get(const std::string& path)
{
	MenuElementPtr element = _root->find(path);

	if (element)
	{
		return element->getWidget();
	}

	rError() << "MenuManager: Warning: Menu " << path << " not found!" << std::endl;
	return nullptr;
}

wxObject* MenuManager::add(const std::string& insertPath,
							const std::string& name,
							eMenuItemType type,
							const std::string& caption,
							const std::string& icon,
							const std::string& eventName)
{
	return new wxMenu;
	return nullptr;
	// TODO
#if 0
	// Sanity check for empty menu
	if (_root == NULL) return NULL;

	MenuElementPtr found = _root->find(insertPath);

	if (found != NULL)
	{
		// Allocate a new MenuItem
		MenuElementPtr newItem = MenuElementPtr(new MenuElement(found));

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
		MenuElementPtr newItem = MenuElementPtr(new MenuElement(_root));

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
#endif
}

wxObject* MenuManager::insert(const std::string& insertPath,
						 const std::string& name,
						 eMenuItemType type,
						 const std::string& caption,
						 const std::string& icon,
						 const std::string& eventName)
{
	return nullptr;
	// TODO
#if 0
	// Sanity check for empty menu
	if (_root == NULL) return NULL;

	MenuElementPtr found = _root->find(insertPath);

	if (found != NULL)
	{
		if (found->parent() != NULL)
		{
			// Get the Menu position of the child widget
			int position = found->parent()->getMenuPosition(found);

			// Allocate a new MenuItem
			MenuElementPtr newItem = MenuElementPtr(new MenuElement(found->parent()));
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
#endif
}

void MenuManager::remove(const std::string& path)
{
	// TODO
#if 0
	// Sanity check for empty menu
	if (_root == NULL) return;

	MenuElementPtr item = _root->find(path);

	if (item == NULL) return; // nothing to do

	MenuElementPtr parent = item->parent();

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
		static_cast<wxMenu*>(item->getWidget());

		int oldPosition = parent->getMenuPosition(item);

		if (oldPosition != -1)
		{
			// Disconnect the item safely before going ahead
			item->removeAllChildren();
			item->setWidget(NULL);

			wxMenu* menu = parentBar->Remove(oldPosition);
			delete menu;
		}
		else
		{
			rWarning() << "Cannot remove menu from menu bar, menu position not found." << std::endl;
		}
	}

	// Remove the found item from the parent menu item
	parent->removeChild(item);
#endif
}

MenuElementPtr MenuManager::findParentMenu(const MenuElementPtr& element)
{
	if (!element) return MenuElementPtr();

	MenuElementPtr parent = element->getParent();

	while (parent)
	{
		if (std::dynamic_pointer_cast<MenuFolder>(parent))
		{
			return parent;
		}
	}

	return MenuElementPtr();
}

} // namespace ui
