#include "MenuBar.h"

#include <wx/menu.h>
#include <wx/frame.h>
#include <wx/wupdlock.h>

#include <sigc++/bind.h>
#include "MenuFolder.h"

namespace ui
{

MenuBar::MenuBar() :
	_menuBar(nullptr)
{}

wxMenuBar* MenuBar::getMenuBar()
{
	if (_menuBar == nullptr)
	{
		construct();
	}

	return _menuBar;
}

void MenuBar::ensureMenusConstructed()
{
	construct();
}

void MenuBar::construct()
{
	_needsRefresh = false;

	if (_menuBar != nullptr)
	{
		// Block redraws for the moment being
		wxWindowUpdateLocker noUpdates(_menuBar);

		MenuElement::constructChildren();
		return;
	}

	_menuBar = new wxMenuBar;

	// Set up the listener ensuring that the opened menu is up to date
	_menuBar->Bind(wxEVT_MENU_OPEN, sigc::mem_fun(this, &MenuBar::onMenuOpen));

	MenuElement::constructChildren();
}

void MenuBar::deconstruct()
{
	MenuElement::deconstructChildren();

	if (_menuBar != nullptr)
	{
		// Any parent frame needs to know about the destruction beforehand
		// deleting the wxMenuBar object doesn't notify its parent
		if (_menuBar->GetFrame())
		{
			_menuBar->GetFrame()->SetMenuBar(nullptr);
		}

		delete _menuBar;
		_menuBar = nullptr;
	}
}

MenuElementPtr MenuBar::findMenu(wxMenu* menu)
{
	for (const MenuElementPtr& candidate : _children)
	{
		if (!std::dynamic_pointer_cast<MenuFolder>(candidate)) continue;

		if (std::static_pointer_cast<MenuFolder>(candidate)->getMenu() == menu)
		{
			return candidate;
		}
	}

	return MenuElementPtr();
}

void MenuBar::onMenuOpen(wxMenuEvent& ev)
{
	// Block redraws for the moment being
	wxWindowUpdateLocker noUpdates(_menuBar);

	MenuElementPtr menu = findMenu(ev.GetMenu());

	if (menu && menu->needsRefresh() && std::dynamic_pointer_cast<MenuFolder>(menu))
	{
		// Rebuild this entire menu
		std::static_pointer_cast<MenuFolder>(menu)->refresh();
	}
}

}
