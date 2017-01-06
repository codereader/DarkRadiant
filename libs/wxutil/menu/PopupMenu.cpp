#include "PopupMenu.h"

namespace wxutil
{

// Default constructor
PopupMenu::PopupMenu() :
	wxMenu()
{
	Connect(wxEVT_MENU, wxCommandEventHandler(PopupMenu::_onItemClick), NULL, this);
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
	addItem(ui::IMenuItemPtr(new wxutil::MenuItem(widget, callback, sensTest, visTest)));
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

// Show the menu
void PopupMenu::show(wxWindow* parent)
{
	// Iterate through the list of MenuItems, enabling or disabling each widget
	// based on its SensitivityTest
	for (MenuItemList::iterator i = _menuItems.begin();
		 i != _menuItems.end();
		 ++i)
	{
		ui::IMenuItem& item = *(*i);

		bool visible = item.isVisible();

		if (visible)
		{
			// Visibility check passed
			item.getMenuItem()->Enable(item.isSensitive());
		}
		else
		{
			// Visibility check failed, skip sensitivity check
			item.getMenuItem()->Enable(false);
		}
	}

	parent->PopupMenu(this);
}

void PopupMenu::_onItemClick(wxCommandEvent& ev)
{
	int commandId = ev.GetId();

	// Find the menu item with that ID
	for (MenuItemList::iterator i = _menuItems.begin();
		 i != _menuItems.end();
		 ++i)
	{
		ui::IMenuItem& item = *(*i);

		if (item.getMenuItem()->GetId() == commandId)
		{
			item.execute();
			break;
		}
	}
}

void PopupMenu::foreachMenuItem(const std::function<void(const ui::IMenuItemPtr&)>& functor)
{
	for (const ui::IMenuItemPtr& item : _menuItems)
	{
		functor(item);
	}
}

} // namespace
