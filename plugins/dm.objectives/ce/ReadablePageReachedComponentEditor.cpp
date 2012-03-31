#include "ReadablePageReachedComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include "i18n.h"
#include <gtkmm/spinbutton.h>
#include "string/convert.h"

namespace objectives
{

namespace ce
{

// Registration helper, will register this editor in the factory
ReadablePageReachedComponentEditor::RegHelper ReadablePageReachedComponentEditor::regHelper;

// Constructor
ReadablePageReachedComponentEditor::ReadablePageReachedComponentEditor(Component& component) :
	_component(&component),
	_readableSpec(Gtk::manage(new SpecifierEditCombo(SpecifierType::SET_READABLE())))
{
	_pageNum = Gtk::manage(new Gtk::SpinButton(*Gtk::manage(new Gtk::Adjustment(1, 1, 65535, 1)), 0, 0));

	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Readable:") + "</b>")), false, false, 0);
	pack_start(*_readableSpec, true, true, 0);
	pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Page Number:"))), false, false, 0);
	pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_pageNum)), false, false, 0);

    // Populate the SpecifierEditCombo with the first specifier
    _readableSpec->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin button with the value from the first component argument
	_pageNum->set_value(string::convert<double>(component.getArgument(0)));
}

// Write to component
void ReadablePageReachedComponentEditor::writeToComponent() const
{
    assert(_component);

    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _readableSpec->getSpecifier()
    );

	_component->setArgument(0, string::to_string(_pageNum->get_value()));
}

} // namespace ce

} // namespace objectives
