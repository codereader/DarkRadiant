#include "PopupMenu.h"
#include "../IconTextMenuItem.h"

namespace gtkutil
{

// Default constructor
PopupMenu::PopupMenu(Gtk::Widget* widget) :
	Gtk::Menu()
{
	// If widget is non-NULL, connect to button-release-event
	if (widget != NULL)
	{
		_buttonReleaseHandler = widget->signal_button_release_event().connect(
			sigc::mem_fun(*this, &PopupMenu::_onClick));
	}
}

PopupMenu::~PopupMenu()
{
	_buttonReleaseHandler.disconnect();
}

// Add a named menu item
void PopupMenu::addItem(Gtk::MenuItem* widget,
						const Callback& callback,
						const SensitivityTest& sensTest,
						const VisibilityTest& visTest)
{
	// Construct a wrapper and pass to specialised method
	addItem(ui::IMenuItemPtr(new MenuItem(widget, callback, sensTest, visTest)));
}

void PopupMenu::addItem(const ui::IMenuItemPtr& item)
{
	_menuItems.push_back(item);

	// Add the GtkWidget to the GtkMenu
	append(*item->getWidget());
}

// Show the menu
void PopupMenu::show()
{
	// Show all elements as first measure
	show_all();

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
			item.getWidget()->show();
			item.getWidget()->set_sensitive(item.isSensitive());
		}
		else
		{
			// Visibility check failed, skip sensitivity check
			item.getWidget()->hide();
		}
	}

	popup(1, gtk_get_current_event_time());
}

// Mouse click callback
bool PopupMenu::_onClick(GdkEventButton* e)
{
	if (e->button == 3) // right-click only
	{
		show();
	}

	return false;
}

} // namespace
