#pragma once

#include "MenuElement.h"
#include <wx/menu.h>
#include <wx/menuitem.h>

namespace ui
{

class MenuFolder :
	public MenuElement
{
private:
	wxMenu* _menu;

	// If this is a submenu, we have a parent menu item
	wxMenuItem* _parentItem;

public:
	MenuFolder();

	virtual wxMenu* getMenu();

	// Empties this menu and rebuilds the wxWidget objects
	// Clears the needsRefresh flag on this object and all children
	void refresh();

protected:
	virtual void construct() override;
	virtual void deconstruct() override;
};

}
