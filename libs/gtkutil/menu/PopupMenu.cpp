#include "PopupMenu.h"
#include "../IconTextMenuItem.h"

namespace gtkutil {

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
			 			Callback callback,
			 			SensitivityTest test)
{
	// Create a MenuItem and add it to the list
	MenuItem item(widget, callback, test);
	_menuItems.push_back(item);
	
	// Connect up the activation callback to GTK.
	g_signal_connect(
		G_OBJECT(widget), // the actual GtkWidget* 
		"activate", 
		G_CALLBACK(_onActivate),
		&_menuItems.back() // pointer to our stored MenuItem struct
	);
	
	// Add the GtkWidget to the GtkMenu
	gtk_menu_shell_append(GTK_MENU_SHELL(_menu), widget);
}

// Show the menu
void PopupMenu::show() 
{
	// Iterate through the list of MenuItems, enabling or disabling each widget
	// based on its SensitivityTest
	for (MenuItemList::iterator i = _menuItems.begin();
		 i != _menuItems.end();
		 ++i)
	{
		if (i->test())
			gtk_widget_set_sensitive(i->widget, TRUE);
		else
			gtk_widget_set_sensitive(i->widget, FALSE);
	}
	
	// Show all elements and display the menu
	gtk_widget_show_all(_menu);
	gtk_menu_popup(
		GTK_MENU(_menu), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME
	);
}

// Mouse click callback
gboolean PopupMenu::_onClick(GtkWidget* w, GdkEventButton* e, PopupMenu* self) {
	if (e->button == 3) { // right-click only
		self->show();
	}
	return FALSE;
}

}
