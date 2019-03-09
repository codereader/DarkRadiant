#pragma once

#include "MenuElement.h"

#include <wx/menuitem.h>

namespace ui
{

class MenuItem :
	public MenuElement
{
private:
	wxMenuItem* _menuItem;

public:
	MenuItem();

	virtual wxMenuItem* getMenuItem();

protected:
	virtual void construct() override;
	virtual void deconstruct() override;
};

}
