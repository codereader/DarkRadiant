#pragma once

#include "MenuElement.h"

namespace ui
{

namespace menu
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

	ItemType getType() const override
	{
		return ItemType::Root;
	}

protected:
	void construct() override
	{}

	void deconstruct() override
	{}
};

}

}
