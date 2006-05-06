#include "TextPropertyEditor.h"

#include <gtk/gtk.h>

namespace ui
{

// Constructor. Create the GTK widgets here

TextPropertyEditor::TextPropertyEditor(Entity* entity, const char* name) {
	std::cout << "Creating a new TextPropertyEditor" << std::endl;
	_widget = gtk_label_new("TextPropertyEditor");
	gtk_widget_show(_widget);
}

// Blank constructor

TextPropertyEditor::TextPropertyEditor() {}

// Destructor. Hide and destory the GTK widgets

TextPropertyEditor::~TextPropertyEditor() {
	std::cout << "Destroying a TextPropertyEditor" << std::endl;
	gtk_widget_hide(_widget);
	gtk_widget_destroy(_widget);
}

}
