#pragma once

#include "MenuElement.h"
#include <wx/menuitem.h>

namespace ui
{

class MenuSeparator :
	public MenuElement
{
private:
	wxMenuItem* _separator;

public:
	MenuSeparator();

	virtual wxMenuItem* getMenuItem();

	eMenuItemType getType() const override
	{
		return menuSeparator;
	}

protected:
	virtual void construct() override;
	virtual void deconstruct() override;
};

}
