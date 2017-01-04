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

	virtual wxMenuItem* getWidget() override;

	virtual void deconstruct() override;

protected:
	virtual void constructWidget() override;
};

}
