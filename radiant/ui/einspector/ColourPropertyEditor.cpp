#include "ColourPropertyEditor.h"

#include "ientity.h"

#include <gtkmm/colorbutton.h>
#include <gtkmm/box.h>
#include <boost/format.hpp>
#include <sstream>

namespace ui
{

// Blank ctor
ColourPropertyEditor::ColourPropertyEditor() :
	_colorButton(NULL)
{}

// Main ctor
ColourPropertyEditor::ColourPropertyEditor(Entity* entity,
										   const std::string& name)
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 6);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Create the colour button
	_colorButton = Gtk::manage(new Gtk::ColorButton);
	_colorButton->signal_color_set().connect(sigc::mem_fun(*this, &ColourPropertyEditor::_onColorSet));

	mainVBox->pack_start(*_colorButton, true, true, 0);

	// Set colour button's colour
	setColourButton(_entity->getKeyValue(name));
}

// Set displayed colour from the keyvalue
void ColourPropertyEditor::setColourButton(const std::string& val)
{
	float r = 0.0, g = 0.0, b = 0.0;
	std::stringstream str(val);

	// Stream the whitespace-separated string into floats
	str >> r;
	str >> g;
	str >> b;

	// Construct the GdkColor and set the GtkColorButton from it
	Gdk::Color col;
	col.set_rgb_p(r, g, b);

	_colorButton->set_color(col);
}

// Get selected colour
std::string ColourPropertyEditor::getSelectedColour()
{
	// Get colour from the button
	Gdk::Color col = _colorButton->get_color();

	// Format the string value appropriately.
	return (boost::format("%.2f %.2f %.2f")
			% (col.get_red()/65535.0)
			% (col.get_green()/65535.0)
			% (col.get_blue()/65535.0)).str();
}

void ColourPropertyEditor::_onColorSet()
{
	// Set the new keyvalue on the entity
	setKeyValue(_key, getSelectedColour());
}


} // namespace ui

