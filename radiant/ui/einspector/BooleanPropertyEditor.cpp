#include "BooleanPropertyEditor.h"

#include <gtk/gtk.h>

namespace ui
{

// Blank ctor

BooleanPropertyEditor::BooleanPropertyEditor() {}

// Constructor. Create the GTK widgets here

BooleanPropertyEditor::BooleanPropertyEditor(Entity* entity, const std::string& name):
	PropertyEditor(entity, name) 
{

	GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);

	_checkBox = gtk_check_button_new_with_label(name.c_str());
	
	gtk_box_pack_start(GTK_BOX(editBox), _checkBox, TRUE, FALSE, 0);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  editBox);
}

// Destructor.

BooleanPropertyEditor::~BooleanPropertyEditor() {
}

// Refresh and commit

void BooleanPropertyEditor::setValue(const std::string& val) {
    if (val == "1") {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_checkBox), TRUE);   
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_checkBox), FALSE);   
    }        
}

const std::string BooleanPropertyEditor::getValue() {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_checkBox))) {
        return "1";
    } else {
        return "0";
    }
}

}
