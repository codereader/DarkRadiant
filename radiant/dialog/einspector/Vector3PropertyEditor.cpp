#include "Vector3PropertyEditor.h"
#include "EntityKeyValueVisitor.h"

#include "exception/InvalidKeyException.h"

#include <gtk/gtk.h>

#include <sstream>
#include <vector>

namespace ui
{

// Blank ctor

Vector3PropertyEditor::Vector3PropertyEditor() {}

// Constructor. Create the GTK widgets here

Vector3PropertyEditor::Vector3PropertyEditor(Entity* entity, const std::string& name):
	PropertyEditor(entity, name, "vector3") 
{
	GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);

    // Create the spin buttons and set them to 0

	_xValue = gtk_spin_button_new_with_range(-32767, 32767, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(_xValue), 0);

    _yValue = gtk_spin_button_new_with_range(-32767, 32767, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(_yValue), 0);

    _zValue = gtk_spin_button_new_with_range(-32767, 32767, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(_zValue), 0);
	
    // Add the spin buttons to the HBox with labels
    
	gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new("X: "), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(editBox), _xValue, TRUE, TRUE, 0);
	
    gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(" Y: "), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(editBox), _yValue, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(" Z: "), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(editBox), _zValue, TRUE, TRUE, 0);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  editBox);
    refresh();
}

// Refresh and commit

void Vector3PropertyEditor::setValue(const std::string& val) {
    // Stream the given string into a temporary buffer to compile a vector
    // of 3 components (separated by spaces in the input).
    std::stringstream stream(val);
    std::vector<float> values;
    float buf;
    
    while (stream >> buf)
        values.push_back(buf);
    
    // Set the Gtk widgets
    if (values.size() == 3) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(_xValue), values[0]);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(_yValue), values[1]);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(_zValue), values[2]);
    }        
            
}

const std::string Vector3PropertyEditor::getValue() {
    std::stringstream stream;
    stream << gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(_xValue)) << " ";
    stream << gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(_yValue)) << " ";
    stream << gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(_zValue));
    return stream.str();
}

}
