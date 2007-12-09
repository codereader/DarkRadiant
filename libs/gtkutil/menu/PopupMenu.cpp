#include "PopupMenu.h"
#include "../IconTextMenuItem.h"

namespace gtkutil {

// Default constructor
PopupMenu::PopupMenu()
: _menu(gtk_menu_new())
{ }

// Add a named menu item
void PopupMenu::addItem(GdkPixbuf* icon, 
			 			const std::string& text,
			 			Callback callback,
			 			SensitivityTest test)
{
	// Pack the icon and text into a widget
	GtkWidget* widget = IconTextMenuItem(icon, text);
	
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

}
