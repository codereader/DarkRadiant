#include "InfoLocationComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "i18n.h"
#include <wx/stattext.h>

namespace objectives 
{

namespace ce 
{

// Registration helper, will register this editor in the factory
InfoLocationComponentEditor::RegHelper InfoLocationComponentEditor::regHelper;

// Constructor
InfoLocationComponentEditor::InfoLocationComponentEditor(wxWindow* parent, Component& component) :
	ComponentEditorBase(parent),
	_component(&component),
	_entSpec(new SpecifierEditCombo(_panel, getChangeCallback())),
	_locationSpec(new SpecifierEditCombo(_panel, getChangeCallback(), SpecifierType::SET_LOCATION()))
{
	wxStaticText* label = new wxStaticText(_panel, wxID_ANY, _("Entity:"));
	label->SetFont(label->GetFont().Bold());

	_panel->GetSizer()->Add(label, 0, wxBOTTOM | wxEXPAND, 6);
	_panel->GetSizer()->Add(_entSpec, 0, wxBOTTOM | wxEXPAND, 6);

	label = new wxStaticText(_panel, wxID_ANY, _("Location:"));
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
void InfoLocationComponentEditor::writeToComponent() const
{
    if (!_active) return; // still under construction

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
