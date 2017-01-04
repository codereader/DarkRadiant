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

	wxObject* getWidget() override
	{
		return nullptr;
	}

protected:
	void construct() override
	{}

	void deconstruct() override
	{}
};

}
