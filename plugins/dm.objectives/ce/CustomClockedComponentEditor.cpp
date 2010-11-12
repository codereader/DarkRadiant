#include "CustomClockedComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include "i18n.h"
#include <gtkmm/entry.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/box.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
CustomClockedComponentEditor::RegHelper CustomClockedComponentEditor::regHelper;

// Constructor
CustomClockedComponentEditor::CustomClockedComponentEditor(Component& component) :
	_component(&component),
	_scriptFunction(Gtk::manage(new Gtk::Entry))
{
	_interval = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(1, 0, 65535, 0.1)), 0, 2)
	);

	pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Script Function:") + "</b>")),
        false, false, 0
    );
	pack_start(*_scriptFunction, false, false, 0);

	pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Clock interval:") + "</b>")),
        false, false, 0
    );

	Gtk::HBox* hbox2 = Gtk::manage(new Gtk::HBox(false, 6));

	hbox2->pack_start(*_interval, false, false, 0);
	hbox2->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("seconds"))), false, false, 0);

	pack_start(*hbox2, false, false, 0);

	// Load the initial values into the boxes
	_scriptFunction->set_text(component.getArgument(0));

	float interval = component.getClockInterval();
	_interval->set_value(interval >= 0 ? interval : 1.0);
}

// Write to component
void CustomClockedComponentEditor::writeToComponent() const
{
    assert(_component);

	_component->setArgument(0, _scriptFunction->get_text());
	_component->setClockInterval(static_cast<float>(_interval->get_value()));
}

} // namespace ce

} // namespace objectives
