#include "PickpocketComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "string/convert.h"

#include "i18n.h"
#include <gtkmm/spinbutton.h>

namespace objectives
{

namespace ce
{

// Registration helper, will register this editor in the factory
PickpocketComponentEditor::RegHelper PickpocketComponentEditor::regHelper;

// Constructor
PickpocketComponentEditor::PickpocketComponentEditor(Component& component) :
	_component(&component),
	_itemSpec(Gtk::manage(new SpecifierEditCombo(SpecifierType::SET_ITEM())))
{
	_amount = Gtk::manage(new Gtk::SpinButton(*Gtk::manage(new Gtk::Adjustment(1, 0, 65535, 1)), 0, 0));

	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Item:") + "</b>")), false, false, 0);
	pack_start(*_itemSpec, false, false, 0);

	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Amount:") + "</b>")), false, false, 0);
	pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_amount)), false, false, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _itemSpec->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin button with the value from the first component argument
	_amount->set_value(string::convert<double>(component.getArgument(0)));
}

// Write to component
void PickpocketComponentEditor::writeToComponent() const
{
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _itemSpec->getSpecifier()
    );

	_component->setArgument(0,
		string::to_string(_amount->get_value()));
}

} // namespace ce

} // namespace objectives
