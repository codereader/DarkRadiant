#pragma once

#include "MenuElement.h"
#include <wx/menuitem.h>

namespace ui
{

namespace menu
{

class MenuSeparator :
	public MenuElement
{
private:
	wxMenuItem* _separator;

public:
	MenuSeparator();

	virtual wxMenuItem* getMenuItem();

	ItemType getType() const override
	{
		return ItemType::Separator;
	}

protected:
	virtual void construct() override;
	virtual void deconstruct() override;
};

}

}

