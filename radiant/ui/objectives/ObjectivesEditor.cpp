#include "ObjectivesEditor.h"

#include <gtk/gtk.h>

namespace ui
{

// Constructor creates widgets
ObjectivesEditor::ObjectivesEditor()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
}

// Show the dialog
void ObjectivesEditor::show() {
	gtk_widget_show_all(_widget);	
}

// Static method to display dialog
void ObjectivesEditor::displayDialog() {
	
	// Static dialog instance
	static ObjectivesEditor _instance;
	
	// Show the instance
	_instance.show();
}

}
