#pragma once

#include "MenuElement.h"

namespace ui
{

class MenuRootElement :
	public MenuElement
{
public:
	MenuRootElement()
	{
		setType(menuRoot);
	}

	~MenuRootElement()
	{
		deconstructChildren();
	}

protected:
	void construct() override
	{}

	void deconstruct() override
	{}
};

}
