#include "PopupMenu.h"
#include "SeparatorItem.h"

namespace wxutil
{

// Default constructor
PopupMenu::PopupMenu() :
	wxMenu()
{
	Bind(wxEVT_MENU, &PopupMenu::_onItemClick, this);
}

PopupMenu::~PopupMenu()
{}

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
    addItem(std::make_shared<SeparatorItem>(
        new wxMenuItem(this, wxID_ANY, wxEmptyString, wxEmptyString, wxITEM_SEPARATOR))
    );
}

void PopupMenu::attachItem(const ui::IMenuItemPtr& item, int position)
{
    if (item->getMenuItem()->GetMenu() == nullptr)
    {
        Insert(position, item->getMenuItem());
    }
}

void PopupMenu::detachItem(const ui::IMenuItemPtr& item)
{
    if (item->getMenuItem()->GetMenu() != nullptr)
    {
        Remove(item->getMenuItem());
    }
}

bool PopupMenu::itemIsVisible(int index, int menuPosition)
{
    const auto& item = _menuItems[index];

    // Call the user visibility check
    bool visible = item->isVisible();

    if (!visible)
    {
        // Item is hidden
        return false;
    }

    // Special treatment for separators
    if (item->getMenuItem()->IsSeparator())
    {
        // Trim separators from the beginning or the end of the menu
        if (menuPosition == 0 || index + 1 >= _menuItems.size())
        {
            return false;
        }

        // Skip all consecutive seprators except for the last
        if (index + 1 < _menuItems.size() && _menuItems[index + 1]->getMenuItem()->IsSeparator())
        {
            return false;
        }
    }

    return true;
}

void PopupMenu::show(wxWindow* parent)
{
	// Iterate through the list of MenuItems, showing/hiding and enabling/disabling each widget
	// based on its test functions
    int position = 0;

	for (auto i = 0; i < _menuItems.size(); ++i)
	{
        const auto& item = _menuItems[i];

        if (!itemIsVisible(i, position))
        {
            // Visibility check failed, detach and skip sensitivity check
            // Don't increase the position counter
            detachItem(item);
            continue;
        }

		// Make sure visible items are attached, increase position
        attachItem(item, position++);

        // Perform the sensitivity check on visible items
		item->getMenuItem()->Enable(item->isSensitive());
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
