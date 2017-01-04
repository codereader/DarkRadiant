#include "MenuBar.h"

#include <wx/menu.h>

namespace ui
{

wxMenuBar* MenuBar::getWidget()
{
	if (_menuBar == nullptr)
	{
		constructWidget();
	}

	return _menuBar;
}

void MenuBar::constructWidget()
{
	if (_menuBar != nullptr)
	{
		MenuElement::constructWidget();
		return;
	}

	_menuBar = new wxMenuBar;

	MenuElement::constructWidget();
}

}
