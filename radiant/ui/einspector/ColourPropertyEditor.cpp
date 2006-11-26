#include "ColourPropertyEditor.h"

#include <gtk/gtk.h>

#include <boost/format.hpp>

#include <sstream>
#include <iostream>

namespace ui
{

// Blank ctor
ColourPropertyEditor::ColourPropertyEditor() {}

// Main ctor
ColourPropertyEditor::ColourPropertyEditor(Entity* entity, const std::string& name)
: PropertyEditor(entity, name) 
{
	_colorButton = gtk_color_button_new();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  _colorButton);
}

// Set displayed value
void ColourPropertyEditor::setValue(const std::string& val) {
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

// Get current value
const std::string ColourPropertyEditor::getValue() {

	// Get colour from the button
	GdkColor col;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(_colorButton), &col);

	// Format the string value appropriately.
	return (boost::format("%.2f %.2f %.2f") 
			% (col.red/65535.0)
			% (col.green/65535.0)
			% (col.blue/65535.0)).str();
}
	
} // namespace ui

