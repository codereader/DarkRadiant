#pragma once

#include "MenuElement.h"
#include <wx/menu.h>

namespace ui
{

class MenuBar :
	public MenuElement
{
private:
	wxMenuBar* _menuBar;

public:
	MenuBar();

	virtual wxMenuBar* getWidget() override;

protected:
	virtual void construct() override;
	virtual void deconstruct() override;

private:
	MenuElementPtr findMenu(wxMenu* menu);
};

}

