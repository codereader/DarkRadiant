#include "BooleanPropertyEditor.h"

#include "ientity.h"

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>

namespace ui
{

// Blank ctor
BooleanPropertyEditor::BooleanPropertyEditor() :
	_checkBox(NULL)
{}

// Constructor. Create the GTK widgets here
BooleanPropertyEditor::BooleanPropertyEditor(Entity* entity,
											 const std::string& name)
: PropertyEditor(entity),
  _checkBox(NULL),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 6);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	Gtk::HBox* editBox = Gtk::manage(new Gtk::HBox(false, 3));
	editBox->set_border_width(3);

	// Create the checkbox with correct initial state, and connect up the
	// toggle callback
	_checkBox = Gtk::manage(new Gtk::CheckButton(name));
	_checkBox->set_active(_entity->getKeyValue(_key) == "1");
	_checkBox->signal_toggled().connect(sigc::mem_fun(*this, &BooleanPropertyEditor::_onToggle));

	editBox->pack_start(*_checkBox, true, false, 0);
	mainVBox->pack_start(*editBox, true, true, 0);
}

void BooleanPropertyEditor::_onToggle()
{
	// Set the key based on the checkbutton state
	setKeyValue(_key, _checkBox->get_active() ? "1" : "0");
}

} // namespace
