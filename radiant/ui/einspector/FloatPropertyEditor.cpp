#include "FloatPropertyEditor.h"

#include "ientity.h"
#include "gtkutil/RightAlignment.h"

#include <iostream>
#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/scale.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

namespace ui
{

FloatPropertyEditor::FloatPropertyEditor() :
	_scale(NULL)
{}

// Main constructor
FloatPropertyEditor::FloatPropertyEditor(Entity* entity,
										 const std::string& key,
										 const std::string& options)
: PropertyEditor(entity),
  _scale(NULL),
  _key(key)
{
	// Construct the main widget (will be managed by the base class)
	Gtk::VBox* mainVBox = new Gtk::VBox(false, 6);
	mainVBox->set_border_width(6);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Split the options string to get min and max values
	std::vector<std::string> values;
	boost::algorithm::split(values, options, boost::algorithm::is_any_of(","));
	if (values.size() != 2)
		return;

	// Attempt to cast to min and max floats
	float min, max;
	try {
		min = boost::lexical_cast<float>(values[0]);
		max = boost::lexical_cast<float>(values[1]);
	}
	catch (boost::bad_lexical_cast&) {
		std::cerr
			<< "[radiant] FloatPropertyEditor failed to parse options string "
			<< "\"" << options << "\"" << std::endl;
		return;
	}

	// Create the HScale and pack into widget
	_scale = Gtk::manage(new Gtk::HScale(min, max, 1.0));

	mainVBox->pack_start(*_scale, false, false, 0);

	// Set the initial value if the entity has one
	float value = 0;
	try
	{
		value = boost::lexical_cast<float>(_entity->getKeyValue(_key));
	}
	catch (boost::bad_lexical_cast&) { }

	_scale->set_value(value);

	// Create and pack in the Apply button
	Gtk::Button* applyButton = Gtk::manage(new Gtk::Button(Gtk::Stock::APPLY));
	applyButton->signal_clicked().connect(sigc::mem_fun(*this, &FloatPropertyEditor::_onApply));

	mainVBox->pack_end(*Gtk::manage(new gtkutil::RightAlignment(*applyButton)), false, false, 0);
}

void FloatPropertyEditor::_onApply()
{
	float value = static_cast<float>(_scale->get_value());

	setKeyValue(_key, boost::lexical_cast<std::string>(value));
}

}
