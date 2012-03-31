#include "AlertComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/convert.h"

#include "i18n.h"
#include <gtkmm/spinbutton.h>

namespace objectives {

namespace ce {

// Registration helper
AlertComponentEditor::RegHelper AlertComponentEditor::regHelper;

// Constructor
AlertComponentEditor::AlertComponentEditor(Component& component) :
	_component(&component),
	_targetCombo(Gtk::manage(new SpecifierEditCombo(SpecifierType::SET_STANDARD_AI())))
{
	_amount = Gtk::manage(new Gtk::SpinButton(*Gtk::manage(new Gtk::Adjustment(1, 0, 65535, 1)), 0, 0));
	_alertLevel = Gtk::manage(new Gtk::SpinButton(*Gtk::manage(new Gtk::Adjustment(1, 1, 5, 1)), 0, 0));

	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("AI:") + "</b>")), false, false, 0);
	pack_start(*_targetCombo, true, true, 0);

	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Amount:") + "</b>")), false, false, 0);
	pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_amount)), false, false, 0);

	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Minimum Alert Level:") + "</b>")), false, false, 0);
	pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_alertLevel)), false, false, 0);

	// Populate the SpecifierEditCombo with the first specifier
    _targetCombo->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin buttons with the values from the component arguments
	_amount->set_value(string::convert<double>(component.getArgument(0)));
	_alertLevel->set_value(string::convert<double>(component.getArgument(1)));
}

// Write to component
void AlertComponentEditor::writeToComponent() const
{
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _targetCombo->getSpecifier()
    );

	_component->setArgument(0, string::to_string(_amount->get_value()));
	_component->setArgument(1, string::to_string(_alertLevel->get_value()));
}

} // namespace ce

} // namespace objectives
