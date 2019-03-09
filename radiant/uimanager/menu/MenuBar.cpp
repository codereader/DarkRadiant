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

MenuBar::~MenuBar()
{
	// When destroyed, make sure to unsubscribe from any idle events
	setNeedsRefresh(false);
}

wxMenuBar* MenuBar::getMenuBar()
{
	if (_menuBar == nullptr)
	{
		construct();
	}

	return _menuBar;
}

bool MenuBar::isConstructed()
{
	return _menuBar != nullptr;
}

void MenuBar::setNeedsRefresh(bool needsRefresh)
{
	MenuElement::setNeedsRefresh(needsRefresh);

	// Let's get notified on idle events
	if (_menuBar == nullptr || _menuBar->GetFrame() == nullptr)
	{
		return;
	}

	if (needsRefresh)
	{
		_menuBar->GetFrame()->Connect(wxEVT_IDLE, wxIdleEventHandler(MenuBar::onIdle), nullptr, this);
	}
	else
	{
		_menuBar->GetFrame()->Disconnect(wxEVT_IDLE, wxIdleEventHandler(MenuBar::onIdle), nullptr, this);
	}
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

void MenuBar::onIdle(wxIdleEvent& ev)
{
	if (!needsRefresh())
	{
		ev.Skip();
		return;
	}

	// Unsubscribe from idle events
	setNeedsRefresh(false);

	construct();

	if (_menuBar != nullptr && _menuBar->GetFrame() != nullptr)
	{
		_menuBar->Refresh();
	}
}

}
