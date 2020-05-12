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

	void setAccelerator(const std::string& accelStr) override;

	eMenuItemType getType() const override
	{
		return menuItem;
	}

	bool isToggle() const override;
	void setToggled(bool isToggled) override;

protected:
	virtual void construct() override;
	virtual void deconstruct() override;

private:
	void onMenuItemClicked(wxCommandEvent& ev);
};

}
