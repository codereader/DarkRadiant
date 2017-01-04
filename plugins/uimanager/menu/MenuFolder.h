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

	virtual wxMenu* getWidget() override;

protected:
	virtual void constructWidget() override;
	virtual void deconstruct() override;
};

}
