#include "InfoLocationComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include "i18n.h"

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
InfoLocationComponentEditor::RegHelper InfoLocationComponentEditor::regHelper;

// Constructor
InfoLocationComponentEditor::InfoLocationComponentEditor(Component& component) :
	_component(&component),
	_entSpec(Gtk::manage(new SpecifierEditCombo)),
	_locationSpec(Gtk::manage(new SpecifierEditCombo(SpecifierType::SET_LOCATION())))
{
	pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Entity:") + "</b>")),
        false, false, 0
    );
	pack_start(*_entSpec, false, false, 0);

	pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Location:") + "</b>")),
        false, false, 0
    );

	pack_start(*_locationSpec, false, false, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _entSpec->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	_locationSpec->setSpecifier(
		component.getSpecifier(Specifier::SECOND_SPECIFIER)
    );
}

// Write to component
void InfoLocationComponentEditor::writeToComponent() const
{
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _entSpec->getSpecifier()
    );

	_component->setSpecifier(
		Specifier::SECOND_SPECIFIER, _locationSpec->getSpecifier()
    );
}

} // namespace ce

} // namespace objectives
