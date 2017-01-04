#include "MenuFolder.h"

#include "itextstream.h"
#include <wx/menu.h>
#include <wx/menuitem.h>

#include "MenuBar.h"
#include "MenuFolder.h"

namespace ui
{

wxMenu* MenuFolder::getWidget()
{
	if (_menu == nullptr)
	{
		constructWidget();
	}

	return _menu;
}

void MenuFolder::constructWidget()
{
	if (_menu != nullptr)
	{
		MenuElement::constructWidget();
		return;
	}

	// Get the parent menu
	MenuElementPtr parent = getParent();

	if (!parent)
	{
		rWarning() << "Cannot construct separator without a parent menu" << std::endl;
		return;
	}

	_menu = new wxMenu();

	if (std::dynamic_pointer_cast<MenuBar>(parent))
	{
		wxMenuBar* bar = std::static_pointer_cast<MenuBar>(parent)->getWidget();
		bar->Append(_menu, getCaption());
	}
	else if (std::dynamic_pointer_cast<MenuFolder>(parent))
	{
		wxMenu* parentMenu = std::static_pointer_cast<MenuFolder>(parent)->getWidget();
		parentMenu->AppendSubMenu(_menu, getCaption());
	}

	MenuElement::constructWidget();
}

}
