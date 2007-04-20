#include "Vector3PropertyEditor.h"

#include "ientity.h"
#include "gtkutil/RightAlignment.h"

#include <gtk/gtk.h>
#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>

namespace ui
{

// Blank ctor

Vector3PropertyEditor::Vector3PropertyEditor() {}

// Constructor. Create the GTK widgets here
Vector3PropertyEditor::Vector3PropertyEditor(Entity* entity, 
											 const std::string& name)
: _entity(entity),
  _key(name)
{
	_widget = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
	GtkWidget* editBox = gtk_hbox_new(FALSE, 6);

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

	// Pack edit box into the main widget
	gtk_box_pack_start(GTK_BOX(_widget), editBox, TRUE, TRUE, 0);

	// Create the apply button and add to the VBox
	GtkWidget* applyButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect(
		G_OBJECT(applyButton), "clicked", G_CALLBACK(_onApply), this
	);
	gtk_box_pack_end(GTK_BOX(_widget), gtkutil::RightAlignment(applyButton),
					   FALSE, FALSE, 0);
					   
	// Populate the spin boxes from the keyvalue
	setWidgetsFromKey(_entity->getKeyValue(name));
}

void Vector3PropertyEditor::setWidgetsFromKey(const std::string& val) {
    
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

/* GTK CALLBACKS */

void Vector3PropertyEditor::_onApply(GtkWidget* w, 
									 Vector3PropertyEditor* self)
{
	using boost::lexical_cast;
	using std::string;
	
	// Construct a text value out of the vector components
	std::string value =
		lexical_cast<string>(
			gtk_spin_button_get_value(GTK_SPIN_BUTTON(self->_xValue))) 
		+ " "
		+ lexical_cast<string>(
			gtk_spin_button_get_value(GTK_SPIN_BUTTON(self->_yValue))) 
		+ " "
		+ lexical_cast<string>(
			gtk_spin_button_get_value(GTK_SPIN_BUTTON(self->_zValue)));
	
	// Set the key on the entity
	self->_entity->setKeyValue(self->_key, value);
}

}
