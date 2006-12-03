#include "LightInspector.h"

#include "mainframe.h"

#include <iostream> 

namespace ui
{

/* CONSTANTS */

namespace {
	
	const char* LIGHTINSPECTOR_TITLE = "Light properties";
	
}

// Private constructor creates GTK widgets
LightInspector::LightInspector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Window properties
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), LIGHTINSPECTOR_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    
    // Widget must hide not destroy when closed
    g_signal_connect(G_OBJECT(_widget), 
    				 "delete-event",
    				 G_CALLBACK(gtk_widget_hide_on_delete),
    				 NULL);
	
}

// Show this dialog
void LightInspector::show() {
	gtk_widget_show_all(_widget);	
}

// Static method to display the dialog
void LightInspector::displayDialog() {
	std::cout << "Displaying light insedfs" << std::endl;
	// Static instance
	static LightInspector _instance;

	// Show the instance
	_instance.show();	
}

} // namespace ui
