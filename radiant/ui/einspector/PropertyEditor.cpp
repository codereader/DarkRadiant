#include "PropertyEditor.h"

#include <gtk/gtk.h>
#include <iostream>

namespace ui {

GtkWidget* PropertyEditor::getWidget() {
	// Get the subclass-private widget, show it and return it
	GtkWidget* childWidget = _getInternalWidget();
	gtk_widget_show_all(childWidget);
	return childWidget;
}

} // namespace ui
