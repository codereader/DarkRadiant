#include "PopupMenu.h"

namespace wxutil
{

// Default constructor
PopupMenu::PopupMenu() :
	wxMenu()
{
	Bind(wxEVT_MENU, &PopupMenu::_onItemClick, this);
}

PopupMenu::~PopupMenu()
{
}

// Add a named menu item
void PopupMenu::addItem(wxMenuItem* widget,
						const Callback& callback,
						const SensitivityTest& sensTest,
						const VisibilityTest& visTest)
{
	// Construct a wrapper and pass to specialised method
	addItem(std::make_shared<wxutil::MenuItem>(widget, callback, sensTest, visTest));
}

void PopupMenu::addItem(const ui::IMenuItemPtr& item)
{
	_menuItems.push_back(item);

	// Add the widget to the menu
	Append(item->getMenuItem());
}

void PopupMenu::addSeparator()
{
	AppendSeparator();
}

void PopupMenu::show(wxWindow* parent)
{
	// Iterate through the list of MenuItems, enabling or disabling each widget
	// based on its SensitivityTest
	for (auto item : _menuItems)
	{
		bool visible = item->isVisible();

		if (visible)
		{
			// Visibility check passed
			item->getMenuItem()->Enable(item->isSensitive());
		}
		else
		{
			// Visibility check failed, skip sensitivity check
			item->getMenuItem()->Enable(false);
		}
	}

	parent->PopupMenu(this);
}

void PopupMenu::_onItemClick(wxCommandEvent& ev)
{
	int commandId = ev.GetId();

	// Find the menu item with that ID
	for (auto item : _menuItems)
	{
		if (item->getMenuItem()->GetId() == commandId)
		{
			item->execute();
			break;
		}
	}
}

void PopupMenu::foreachMenuItem(const std::function<void(const ui::IMenuItemPtr&)>& functor)
{
	for (const auto& item : _menuItems)
	{
		functor(item);
	}
}

} // namespace
