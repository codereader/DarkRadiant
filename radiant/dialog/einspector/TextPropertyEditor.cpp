#include "TextPropertyEditor.h"

#include <gtk/gtk.h>

namespace ui
{

// Initialise the Registration Helper
PropertyEditorRegistrationHelper TextPropertyEditor::_helper("text", new TextPropertyEditor());

// Blank ctor

TextPropertyEditor::TextPropertyEditor():
	PropertyEditor(NULL, "", "") {}

// Constructor. Create the GTK widgets here

TextPropertyEditor::TextPropertyEditor(Entity* entity, const std::string& name):
	PropertyEditor(entity, name, "text") 
{
	std::cout << "Creating a new TextPropertyEditor" << std::endl;
	_widget = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), getTitleBox(), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(_widget), getApplyButtonHbox(), FALSE, FALSE, 0);
	gtk_widget_show_all(_widget);
}

// Destructor. Hide and destroy the GTK widgets

TextPropertyEditor::~TextPropertyEditor() {
	std::cout << "Destroying a TextPropertyEditor" << std::endl;
	gtk_widget_hide(_widget);
	gtk_widget_destroy(_widget);
}

}
