#include "AddPropertyDialog.h"

#include <gtk/gtkwindow.h>
#include <gtk/gtkmain.h>

namespace ui
{

// Constructor creates GTK widgets

AddPropertyDialog::AddPropertyDialog()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
}

// Show the widgets and block for a selection

const std::string& AddPropertyDialog::showAndBlock() {
	gtk_widget_show_all(_widget);
	gtk_main();
	return _selectedProperty;	
}

// Static method to create and show an instance, and return the chosen
// property to calling function.

std::string AddPropertyDialog::chooseProperty() {
	static AddPropertyDialog _instance;
	return _instance.showAndBlock();	
}

}
