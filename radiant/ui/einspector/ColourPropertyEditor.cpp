#include "ColourPropertyEditor.h"

#include "ientity.h"

#include <gtk/gtk.h>
#include <boost/format.hpp>
#include <sstream>

namespace ui
{

// Blank ctor
ColourPropertyEditor::ColourPropertyEditor() {}

// Main ctor
ColourPropertyEditor::ColourPropertyEditor(Entity* entity, 
										   const std::string& name)
: _entity(entity),
  _key(name)
{
	_widget = gtk_vbox_new(FALSE, 6);
	
	// Create the colour button
	_colorButton = gtk_color_button_new();
	gtk_box_pack_start(GTK_BOX(_widget), _colorButton, TRUE, TRUE, 0);
	g_signal_connect(
		G_OBJECT(_colorButton), "color-set", G_CALLBACK(_onColorSet), this
	);
	
	// Set colour button's colour
	setColourButton(_entity->getKeyValue(name)); 
}

// Set displayed colour from the keyvalue
void ColourPropertyEditor::setColourButton(const std::string& val) {
	float r = 0.0, g = 0.0, b = 0.0;
	std::stringstream str(val);
	
	// Stream the whitespace-separated string into floats
	str >> r; 
	str >> g;
	str >> b;

	// Construct the GdkColor and set the GtkColorButton from it
	GdkColor col = { 0, guint16(r*65535), guint16(g*65535), guint32(b*65535) };
	gtk_color_button_set_color(GTK_COLOR_BUTTON(_colorButton), &col);	
}

// Get selected colour
std::string ColourPropertyEditor::getSelectedColour() {

	// Get colour from the button
	GdkColor col;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(_colorButton), &col);

	// Format the string value appropriately.
	return (boost::format("%.2f %.2f %.2f") 
			% (col.red/65535.0)
			% (col.green/65535.0)
			% (col.blue/65535.0)).str();
}

/* GTK CALLBACKS */

void ColourPropertyEditor::_onColorSet(GtkWidget* w, ColourPropertyEditor* self)
{
	// Set the new keyvalue on the entity
	self->_entity->setKeyValue(self->_key, self->getSelectedColour());	
}

	
} // namespace ui

