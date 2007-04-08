#include "SoundChooser.h"

#include "mainframe.h"

#include <gtk/gtk.h>

namespace ui
{

// Constructor
SoundChooser::SoundChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Set up the window
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose sound");
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(_widget), GDK_WINDOW_TYPE_HINT_DIALOG);
    
    // Delete event
    g_signal_connect(
    	G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), this
    );
}

// Show and block
std::string SoundChooser::chooseSound() {
	gtk_widget_show_all(_widget);
	gtk_main();
	return "Blah";	
}

/* GTK CALLBACKS */

// Delete dialog
void SoundChooser::_onDelete(GtkWidget* w, SoundChooser* self) {
	gtk_main_quit();	
}

}
