#include "Vector3PropertyEditor.h"

#include "i18n.h"
#include "ientity.h"
#include "gtkutil/RightAlignment.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/label.h>

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
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 6);
	mainVBox->set_border_width(6);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	Gtk::HBox* editBox = Gtk::manage(new Gtk::HBox(false, 6));

    // Create the spin buttons and set them to 0
	Gtk::Adjustment* xAdj = Gtk::manage(new Gtk::Adjustment(0, -32767, 32767, 1));
	Gtk::Adjustment* yAdj = Gtk::manage(new Gtk::Adjustment(0, -32767, 32767, 1));
	Gtk::Adjustment* zAdj = Gtk::manage(new Gtk::Adjustment(0, -32767, 32767, 1));

	_xValue = Gtk::manage(new Gtk::SpinButton(*xAdj));
    _yValue = Gtk::manage(new Gtk::SpinButton(*yAdj));
	_zValue = Gtk::manage(new Gtk::SpinButton(*zAdj));

    // Add the spin buttons to the HBox with labels
	editBox->pack_start(*Gtk::manage(new Gtk::Label(_("X: "))), false, false, 0);
	editBox->pack_start(*_xValue, true, true, 0);

    editBox->pack_start(*Gtk::manage(new Gtk::Label(_(" Y: "))), false, false, 0);
    editBox->pack_start(*_yValue, true, true, 0);

    editBox->pack_start(*Gtk::manage(new Gtk::Label(_(" Z: "))), false, false, 0);
    editBox->pack_start(*_zValue, true, true, 0);

	// Pack edit box into the main widget
	mainVBox->pack_start(*editBox, true, true, 0);

	// Create the apply button and add to the VBox
	Gtk::Button* applyButton = Gtk::manage(new Gtk::Button(Gtk::Stock::APPLY));
	applyButton->signal_clicked().connect(
		sigc::mem_fun(*this, &Vector3PropertyEditor::_onApply));

	mainVBox->pack_end(*Gtk::manage(new gtkutil::RightAlignment(*applyButton)),
					   false, false, 0);

	// Populate the spin boxes from the keyvalue
	setWidgetsFromKey(_entity->getKeyValue(name));
}

void Vector3PropertyEditor::setWidgetsFromKey(const std::string& val)
{
    // Stream the given string into a temporary buffer to compile a vector
    // of 3 components (separated by spaces in the input).
    std::stringstream stream(val);
    std::vector<float> values;
    float buf;

    while (stream >> buf)
        values.push_back(buf);

    // Set the Gtk widgets
    if (values.size() == 3)
	{
		_xValue->set_value(values[0]);
		_yValue->set_value(values[1]);
		_zValue->set_value(values[2]);
    }
}

void Vector3PropertyEditor::_onApply()
{
	using boost::lexical_cast;
	using std::string;

	// Construct a text value out of the vector components
	std::string value =
		lexical_cast<string>(_xValue->get_value())
		+ " "
		+ lexical_cast<string>(_yValue->get_value())
		+ " "
		+ lexical_cast<string>(_zValue->get_value());

	// Set the key on the entity
	setKeyValue(_key, value);
}

}
