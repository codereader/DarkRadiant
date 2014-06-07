#include "LocationComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "i18n.h"
#include <wx/stattext.h>

namespace objectives
{

namespace ce
{

// Registration helper, will register this editor in the factory
LocationComponentEditor::RegHelper LocationComponentEditor::regHelper;

// Constructor
LocationComponentEditor::LocationComponentEditor(wxWindow* parent, Component& component) :
	_component(&component),
	_entSpec(new SpecifierEditCombo(parent)),
	_locationSpec(new SpecifierEditCombo(parent, SpecifierType::SET_LOCATION()))
{
	wxStaticText* label = new wxStaticText(parent, wxID_ANY, _("Entity:"));
	label->SetFont(label->GetFont().Bold());

	_panel->GetSizer()->Add(label, 0, wxBOTTOM | wxEXPAND, 6);
	_panel->GetSizer()->Add(_entSpec, 0, wxBOTTOM | wxEXPAND, 6);

	label = new wxStaticText(parent, wxID_ANY, _("Location:"));
	label->SetFont(label->GetFont().Bold());

	_panel->GetSizer()->Add(label, 0, wxBOTTOM | wxEXPAND, 6);
	_panel->GetSizer()->Add(_locationSpec, 0, wxBOTTOM | wxEXPAND, 6);

    // Populate the SpecifierEditCombo with the first specifier
    _entSpec->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	_locationSpec->setSpecifier(
		component.getSpecifier(Specifier::SECOND_SPECIFIER)
    );
}

// Write to component
void LocationComponentEditor::writeToComponent() const
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
