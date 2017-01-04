#pragma once

#include "MenuElement.h"
#include <wx/menu.h>

namespace ui
{

class MenuFolder :
	public MenuElement
{
private:
	wxMenu* _menu;

public:
	virtual wxMenu* getWidget() override;

protected:
	virtual void constructWidget() override;
};

}
