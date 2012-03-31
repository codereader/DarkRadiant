#include "DistanceComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/convert.h"

#include "i18n.h"
#include <gtkmm/entry.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/box.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
DistanceComponentEditor::RegHelper DistanceComponentEditor::regHelper;

// Constructor
DistanceComponentEditor::DistanceComponentEditor(Component& component) :
	_component(&component),
	_entity(Gtk::manage(new Gtk::Entry)),
	_location(Gtk::manage(new Gtk::Entry))
{
	// Allow for one digit of the distance, everything below this step size is insane
	_distance = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(1, 0, 132000, 1)), 0, 1)
	);

	_interval = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(1, 0, 65535, 0.1)), 0, 2)
	);

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));
	pack_start(*hbox, false, false, 0);

	hbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Entity:"))), false, false, 0);
	hbox->pack_start(*_entity, true, true, 0);

	Gtk::HBox* hbox2 = Gtk::manage(new Gtk::HBox(false, 6));
	pack_start(*hbox2, false, false, 0);

	hbox2->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Location Entity:"))), false, false, 0);
	hbox2->pack_start(*_location, true, true, 0);

	Gtk::HBox* hbox3 = Gtk::manage(new Gtk::HBox(false, 6));
	pack_start(*hbox3, false, false, 0);

	hbox3->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Distance:"))), false, false, 0);
	hbox3->pack_start(*_distance, true, true, 0);

	// The second row contains the clock interval
	Gtk::HBox* hbox4 = Gtk::manage(new Gtk::HBox(false, 6));

	hbox4->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Clock interval:"))), false, false, 0);
	hbox4->pack_start(*_interval, false, false, 0);
	hbox4->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("seconds"))), false, false, 0);

	pack_start(*hbox4, false, false, 0);

	// Load the initial values from the component arguments
	_entity->set_text(component.getArgument(0));
	_location->set_text(component.getArgument(1));

	_distance->set_value(string::convert<double>(component.getArgument(2)));
	float interval = component.getClockInterval();
	_interval->set_value(interval >= 0 ? interval : 1.0);
}

// Write to component
void DistanceComponentEditor::writeToComponent() const
{
    assert(_component);

	_component->setArgument(0, _entity->get_text());
	_component->setArgument(1, _location->get_text());
	_component->setArgument(2, string::to_string(_distance->get_value()));
	_component->setClockInterval(static_cast<float>(_interval->get_value()));
}

} // namespace ce

} // namespace objectives
