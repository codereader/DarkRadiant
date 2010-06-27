#include "PopupMenu.h"
#include "../IconTextMenuItem.h"

namespace gtkutil
{

// Default constructor
PopupMenu::PopupMenu(GtkWidget* widget)
: _menu(gtk_menu_new())
{
	// Take ownership of the menu
	g_object_ref_sink(_menu);
	
	// If widget is non-NULL, connect to button-release-event
	if (widget != NULL) {
		g_signal_connect(
			G_OBJECT(widget), "button-release-event", G_CALLBACK(_onClick), this
		);
	}
}

// Add a named menu item
void PopupMenu::addItem(GtkWidget* widget,
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
	
	// Connect up the activation callback to GTK.
	g_signal_connect(
		G_OBJECT(item->getWidget()), // the actual GtkWidget* 
		"activate", 
		G_CALLBACK(_onActivate),
		item.get() // pointer to our stored IMenuItem
	);
	
	// Add the GtkWidget to the GtkMenu
	gtk_menu_shell_append(GTK_MENU_SHELL(_menu), item->getWidget());
}

// Show the menu
void PopupMenu::show() 
{
	// Show all elements as first measure
	gtk_widget_show_all(_menu);

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
			gtk_widget_show(item.getWidget());

			bool sensitive = item.isSensitive();
			gtk_widget_set_sensitive(item.getWidget(), sensitive ? TRUE : FALSE);
		}
		else
		{
			// Visibility check failed, skip sensitivity check
			gtk_widget_hide(item.getWidget());
		}
	}
	
	gtk_menu_popup(
		GTK_MENU(_menu), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME
	);
}

// Mouse click callback
gboolean PopupMenu::_onClick(GtkWidget* w, GdkEventButton* e, PopupMenu* self)
{
	if (e->button == 3) // right-click only
	{ 
		self->show();
	}

	return FALSE;
}

} // namespace
