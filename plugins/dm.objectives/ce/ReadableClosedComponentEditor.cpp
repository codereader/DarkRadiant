#include "ReadableClosedComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include "i18n.h"
#include "string/string.h"

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
ReadableClosedComponentEditor::RegHelper ReadableClosedComponentEditor::regHelper;

// Constructor
ReadableClosedComponentEditor::ReadableClosedComponentEditor(Component& component) :
	_component(&component),
	_readableSpec(Gtk::manage(new SpecifierEditCombo(SpecifierType::SET_READABLE())))
{
	pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Readable:") + "</b>")),
		false, false, 0
	);
	pack_start(*_readableSpec, true, true, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _readableSpec->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );
}

// Write to component
void ReadableClosedComponentEditor::writeToComponent() const
{
    assert(_component);

    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _readableSpec->getSpecifier()
    );
}

} // namespace ce

} // namespace objectives
