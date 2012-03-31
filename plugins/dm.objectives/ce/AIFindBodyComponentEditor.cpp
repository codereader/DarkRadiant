#include "AIFindBodyComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/convert.h"

#include "i18n.h"
#include <gtkmm/spinbutton.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
AIFindBodyComponentEditor::RegHelper AIFindBodyComponentEditor::regHelper;

// Constructor
AIFindBodyComponentEditor::AIFindBodyComponentEditor(Component& component) :
	_component(&component),
	_bodyCombo(Gtk::manage(new SpecifierEditCombo(SpecifierType::SET_STANDARD_AI())))
{
	_amount = Gtk::manage(new Gtk::SpinButton(*Gtk::manage(new Gtk::Adjustment(1, 0, 65535, 1)), 0, 0));

	// Main vbox
	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Body:") + "</b>")),
        false, false, 0
    );

	pack_start(*_bodyCombo, false, false, 0);
	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Amount:"))), false, false, 0);
	pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_amount)), false, false, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _bodyCombo->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin button with the value from the first component argument
	_amount->set_value(string::convert<double>(component.getArgument(0), 1.0));
}

// Write to component
void AIFindBodyComponentEditor::writeToComponent() const
{
    assert(_component);
	_component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _bodyCombo->getSpecifier()
    );

	_component->setArgument(0, string::to_string(_amount->get_value()));
}

} // namespace ce

} // namespace objectives
