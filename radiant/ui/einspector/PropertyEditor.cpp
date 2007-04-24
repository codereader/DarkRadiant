#include "PropertyEditor.h"

#include <gtk/gtk.h>
#include <iostream>

namespace ui {

PropertyEditor::PropertyEditor() {
	// Register this class as quit function
	_quitHandle = gtk_quit_add(0, onGTKQuit, this);
}

PropertyEditor::~PropertyEditor() {
	// De-register self on destruction
	gtk_quit_remove(_quitHandle);
	
	// Check for a valid widget and destroy it if this is the case
	if (_widget != NULL && GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

GtkWidget* PropertyEditor::getWidget() {
	gtk_widget_show_all(_widget);
	return _widget;
}

gboolean PropertyEditor::onGTKQuit(gpointer data) {
	PropertyEditor* self = reinterpret_cast<PropertyEditor*>(data);
	
	// Invalidate the widget pointer to avoid the destructor from 
	// calling gtk_widget_destroy()
	self->_widget = NULL;
	
	return FALSE;
}

} // namespace ui
