#include "MenuFolder.h"

#include "itextstream.h"
#include <wx/menu.h>
#include <wx/menuitem.h>

#include "MenuBar.h"
#include "MenuFolder.h"

namespace ui
{

MenuFolder::MenuFolder() :
	_menu(nullptr),
	_parentItem(nullptr)
{}

wxMenu* MenuFolder::getWidget()
{
	if (_menu == nullptr)
	{
		constructWidget();
	}

	return _menu;
}

void MenuFolder::constructWidget()
{
	if (_menu != nullptr)
	{
		MenuElement::constructChildren();
		return;
	}

	if (isVisible())
	{
		// Get the parent menu
		MenuElementPtr parent = getParent();

		if (!parent)
		{
			rWarning() << "Cannot construct separator without a parent menu" << std::endl;
			return;
		}

		_menu = new wxMenu();

		if (std::dynamic_pointer_cast<MenuBar>(parent))
		{
			wxMenuBar* bar = std::static_pointer_cast<MenuBar>(parent)->getWidget();
			bar->Append(_menu, getCaption());
		}
		else if (std::dynamic_pointer_cast<MenuFolder>(parent))
		{
			wxMenu* parentMenu = std::static_pointer_cast<MenuFolder>(parent)->getWidget();
			_parentItem = parentMenu->AppendSubMenu(_menu, getCaption());
		}
	}

	MenuElement::constructChildren();
}

void MenuFolder::deconstruct()
{
	// Destruct children first
	MenuElement::deconstructChildren();

	if (_parentItem != nullptr)
	{
		if (_parentItem->GetMenu() != nullptr)
		{
			// This is a submenu, we can just rely on the parent item deleting the _menu object
			_parentItem->GetMenu()->Delete(_parentItem);
		}
		else
		{
			// Unattached parent item, delete it
			delete _parentItem;
		}

		_parentItem = nullptr;

		// Parent item deleted the _menu already in its destructor
		_menu = nullptr;
	}

	if (_menu != nullptr)
	{
		wxMenuBar* menuBar = _menu->GetMenuBar();

		// Check if we're attached to a menu folder
		if (menuBar != nullptr)
		{
			for (std::size_t i = 0; i < menuBar->GetMenuCount(); ++i)
			{
				if (menuBar->GetMenu(i) == _menu)
				{
					// Detach the menu from its bar, caller is responsible of deleting it
					menuBar->Remove(i);
					break;
				}
			}
		}
		
		// Regardless of whether we have a bar attached, we need to delete the menu
		delete _menu;
		_menu = nullptr;
	}
}

}
