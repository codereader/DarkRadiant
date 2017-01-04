#include "MenuItem.h"

#include "itextstream.h"
#include "ieventmanager.h"

#include "../LocalBitmapArtProvider.h"
#include "MenuFolder.h"

namespace ui
{

wxMenuItem* MenuItem::getWidget()
{
	if (_menuItem == nullptr)
	{
		constructWidget();
	}

	return _menuItem;
}

void MenuItem::constructWidget()
{
	if (_menuItem != nullptr)
	{
		MenuElement::constructWidget();
		return;
	}

	// Get the parent menu
	MenuElementPtr parent = getParent();

	if (!parent || !std::dynamic_pointer_cast<MenuFolder>(parent))
	{
		rWarning() << "Cannot construct item without a parent menu" << std::endl;
		return;
	}

	wxMenu* menu = std::static_pointer_cast<MenuFolder>(parent)->getWidget();

	std::string caption = _caption;

	// Try to lookup the event name
	IEventPtr event = GlobalEventManager().findEvent(_event);

	if (!event->empty())
	{
		// Retrieve an accelerator string formatted for a menu
		// greebo: Accelerators seem to globally catch the key events, add a space to fool wxWidgets
		caption = _caption + "\t " + GlobalEventManager().getAcceleratorStr(event, true);
	}
	else
	{
		rWarning() << "MenuElement: Cannot find associated event: " << _event << std::endl;
	}

	// Create a new MenuElement
	_menuItem = new wxMenuItem(nullptr, _nextMenuItemId++, caption);
	
	if (!_icon.empty())
	{
		_menuItem->SetBitmap(wxArtProvider::GetBitmap(LocalBitmapArtProvider::ArtIdPrefix() + _icon));
	}

	_menuItem->SetCheckable(event && event->isToggle());

	menu->Append(_menuItem);

	if (event)
	{
		event->connectMenuItem(_menuItem);
	}
	else
	{
		// No event attached to this menu item, disable it
		menu->Enable(_menuItem->GetId(), false);
	}

	MenuElement::constructWidget();
}

}
