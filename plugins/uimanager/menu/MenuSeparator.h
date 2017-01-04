#pragma once

#include "MenuElement.h"
#include <wx/menuitem.h>

namespace ui
{

class MenuSeparator :
	public MenuElement
{
private:
	wxMenuItem* _separator;

public:
	virtual wxMenuItem* getWidget() override;

protected:
	virtual void constructWidget() override;
};

}
