#pragma once

#include "MenuElement.h"

#include <wx/menuitem.h>

namespace ui
{

namespace menu
{

class MenuItem :
	public MenuElement
{
private:
	wxMenuItem* _menuItem;

public:
	MenuItem();

	virtual wxMenuItem* getMenuItem();

	void setAccelerator(const std::string& accelStr) override;

	ItemType getType() const override
	{
		return ItemType::Item;
	}

	bool isToggle() const override;
	void setToggled(bool isToggled) override;

protected:
	virtual void construct() override;
	virtual void deconstruct() override;
};

}

}
