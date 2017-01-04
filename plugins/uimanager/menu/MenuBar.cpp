#include "MenuBar.h"

#include <wx/menu.h>

#include "MenuFolder.h"

namespace ui
{

MenuBar::MenuBar() :
	_menuBar(nullptr)
{}

wxMenuBar* MenuBar::getWidget()
{
	if (_menuBar == nullptr)
	{
		construct();
	}

	return _menuBar;
}

void MenuBar::construct()
{
	_needsRefresh = false;

	if (_menuBar != nullptr)
	{
		MenuElement::constructChildren();
		return;
	}

	_menuBar = new wxMenuBar;

	// Set up the listener ensuring that the opened menu is up to date
	_menuBar->Bind(wxEVT_MENU_OPEN, [&](wxMenuEvent& ev)
	{
		MenuElementPtr menu = findMenu(ev.GetMenu());

		if (menu && menu->needsRefresh() && std::dynamic_pointer_cast<MenuFolder>(menu))
		{
			// Rebuild this entire menu
			std::static_pointer_cast<MenuFolder>(menu)->refresh();
		}
	});

	MenuElement::constructChildren();
}

void MenuBar::deconstruct()
{
	MenuElement::deconstructChildren();

	if (_menuBar != nullptr)
	{
		delete _menuBar;
		_menuBar = nullptr;
	}
}

MenuElementPtr MenuBar::findMenu(wxMenu* menu)
{
	for (const MenuElementPtr& candidate : _children)
	{
		if (candidate->getWidget() == menu)
		{
			return candidate;
		}
	}

	return MenuElementPtr();
}

}
