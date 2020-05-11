#include "MenuItem.h"

#include "itextstream.h"
#include "ieventmanager.h"

#include "../LocalBitmapArtProvider.h"
#include "MenuFolder.h"

namespace ui
{

MenuItem::MenuItem() :
	_menuItem(nullptr)
{}

wxMenuItem* MenuItem::getMenuItem()
{
	if (_menuItem == nullptr)
	{
		construct();
	}

	return _menuItem;
}

void MenuItem::construct()
{
	if (_menuItem != nullptr)
	{
		MenuElement::constructChildren();
		return;
	}

	if (!isVisible())
	{
		MenuElement::constructChildren();
		return;
	}

	// Get the parent menu
	MenuElementPtr parent = getParent();

	if (!parent || !std::dynamic_pointer_cast<MenuFolder>(parent))
	{
		rWarning() << "Cannot construct item without a parent menu" << std::endl;
		return;
	}

	wxMenu* menu = std::static_pointer_cast<MenuFolder>(parent)->getMenu();

	std::string caption = _caption;

#if 0
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
#endif

	// Create a new MenuElement
	_menuItem = new wxMenuItem(nullptr, _nextMenuItemId++, caption);
	
	if (!_icon.empty())
	{
		_menuItem->SetBitmap(wxArtProvider::GetBitmap(LocalBitmapArtProvider::ArtIdPrefix() + _icon));
	}

	bool isToggle = GlobalEventManager().findEvent(_event)->isToggle();
	_menuItem->SetCheckable(isToggle);

	int pos = parent->getMenuPosition(shared_from_this());
	menu->Insert(pos, _menuItem);

#if 0
	if (event)
	{
		event->connectMenuItem(_menuItem);
	}
	else
	{
		// No event attached to this menu item, disable it
		menu->Enable(_menuItem->GetId(), false);
	}
#endif

#if 1
	GlobalEventManager().registerMenuItem(_event, shared_from_this());
#endif

	MenuElement::constructChildren();
}

void MenuItem::setAccelerator(const std::string& accelStr)
{
	if (_menuItem == nullptr) return;

	std::string caption = _caption + "\t " + accelStr;
	_menuItem->SetItemLabel(caption);
}

void MenuItem::deconstruct()
{
	// Destruct children first
	MenuElement::deconstructChildren();

	if (_menuItem != nullptr)
	{
		// Try to lookup the event name
		if (!_event.empty())
		{
#if 1
			GlobalEventManager().unregisterMenuItem(_event, shared_from_this());
#else
			IEventPtr event = GlobalEventManager().findEvent(_event);

			if (event)
			{
				event->disconnectMenuItem(_menuItem);
			}
#endif
		}

		if (_menuItem->GetMenu() != nullptr)
		{
			_menuItem->GetMenu()->Remove(_menuItem);
		}

		delete _menuItem;
		_menuItem = nullptr;
	}
}

bool MenuItem::isToggle() const
{
	return _menuItem != nullptr && _menuItem->IsCheckable();
}

void MenuItem::setToggled(bool isToggled)
{
	assert(isToggle());

	if (_menuItem != nullptr)
	{
		_menuItem->Check(isToggled);
	}
}

}
