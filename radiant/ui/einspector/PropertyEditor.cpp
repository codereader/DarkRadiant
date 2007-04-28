#include "PropertyEditor.h"

#include <gtk/gtk.h>
#include <iostream>

namespace ui {

GtkWidget* PropertyEditor::getWidget() {
	gtk_widget_show_all(_widget);
	return _widget;
}

} // namespace ui
