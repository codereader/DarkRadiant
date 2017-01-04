#include "MenuBar.h"

#include <wx/menu.h>

namespace ui
{

MenuBar::MenuBar() :
	_menuBar(nullptr)
{}

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
		MenuElement::constructChildren();
		return;
	}

	_menuBar = new wxMenuBar;

	MenuElement::constructChildren();
}

void MenuBar::deconstruct()
{
	MenuElement::deconstructChildren();

	if (_menuBar != nullptr)
	{
		delete _menuBar;
		_menuBar = nullptr;
	}
}

}
