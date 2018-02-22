#include "MenuSeparator.h"

#include "itextstream.h"
#include <wx/menu.h>

#include "MenuFolder.h"

namespace ui
{

MenuSeparator::MenuSeparator() :
	_separator(nullptr)
{}

wxMenuItem* MenuSeparator::getMenuItem()
{
	if (_separator == nullptr)
	{
		construct();
	}

	return _separator;
}

void MenuSeparator::construct()
{
	_needsRefresh = false;

	if (_separator != nullptr)
	{
		MenuElement::constructChildren();
		return;
	}

	if (isVisible())
	{
		// Get the parent menu
		MenuElementPtr parent = getParent();

		if (!parent || !std::dynamic_pointer_cast<MenuFolder>(parent))
		{
			rWarning() << "Cannot construct separator without a parent menu" << std::endl;
			return;
		}

		wxMenu* menu = std::static_pointer_cast<MenuFolder>(parent)->getMenu();

		int pos = parent->getMenuPosition(shared_from_this());
		_separator = menu->InsertSeparator(pos);
	}

	MenuElement::constructChildren();
}

void MenuSeparator::deconstruct()
{
	// Destruct children first
	MenuElement::deconstructChildren();

	if (_separator != nullptr)
	{
		if (_separator->GetMenu() != nullptr)
		{
			_separator->GetMenu()->Remove(_separator);
		}

		delete _separator;
		_separator = nullptr;
	}
}

}
