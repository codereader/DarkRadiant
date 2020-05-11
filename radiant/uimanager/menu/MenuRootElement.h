#pragma once

#include "MenuElement.h"

namespace ui
{

class MenuRootElement :
	public MenuElement
{
public:
	MenuRootElement()
	{}

	~MenuRootElement()
	{
		deconstructChildren();
	}

	eMenuItemType getType() const override
	{
		return menuRoot;
	}

protected:
	void construct() override
	{}

	void deconstruct() override
	{}
};

}
