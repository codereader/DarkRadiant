#include "ReadableOpenedComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "i18n.h"
#include "string/string.h"
#include <wx/stattext.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
ReadableOpenedComponentEditor::RegHelper ReadableOpenedComponentEditor::regHelper;

// Constructor
ReadableOpenedComponentEditor::ReadableOpenedComponentEditor(wxWindow* parent, Component& component) :
	ComponentEditorBase(parent),
	_component(&component),
	_readableSpec(new SpecifierEditCombo(_panel, getChangeCallback(), SpecifierType::SET_READABLE()))
{
	wxStaticText* label = new wxStaticText(_panel, wxID_ANY, _("Readable:"));
	label->SetFont(label->GetFont().Bold());

	_panel->GetSizer()->Add(label, 0, wxBOTTOM | wxEXPAND, 6);
	_panel->GetSizer()->Add(_readableSpec, 0, wxBOTTOM | wxEXPAND, 6);

    // Populate the SpecifierEditCombo with the first specifier
    _readableSpec->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );
}

// Write to component
void ReadableOpenedComponentEditor::writeToComponent() const
{
    if (!_active) return; // still under construction

    assert(_component);

    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _readableSpec->getSpecifier()
    );
}

} // namespace ce

} // namespace objectives
