#include "SkinChooser.h"

#include "groupdialog.h"
#include <gtk/gtkwindow.h>

namespace ui
{

// Constructor
SkinChooser::SkinChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Set up window
	GtkWindow* gd = GroupDialog_getWindow();

	gtk_window_set_transient_for(GTK_WINDOW(_widget), gd);
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose texture");

	// Set the default size of the window
	gint w, h;
	gtk_window_get_size(gd, &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w, h);
	
}

// Show the dialog and block for a selection
std::string SkinChooser::showAndBlock() {
	gtk_widget_show_all(_widget);
	return "_none";
}

// Static method to display singleton instance and choose a skin
std::string SkinChooser::chooseSkin() {
	
	// The static instance
	static SkinChooser _instance;
	
	// Show and block the instance, returning the selected skin
	return _instance.showAndBlock();	
}

}
