#include "AlertComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "string/convert.h"

#include "i18n.h"
#include <wx/spinctrl.h>
#include <wx/stattext.h>

namespace objectives
{

namespace ce
{

// Registration helper
AlertComponentEditor::RegHelper AlertComponentEditor::regHelper;

// Constructor
AlertComponentEditor::AlertComponentEditor(wxWindow* parent, Component& component) :
	ComponentEditorBase(parent),
	_component(&component),
	_targetCombo(new SpecifierEditCombo(_panel, getChangeCallback(), SpecifierType::SET_STANDARD_AI()))
{
	_amount = new wxSpinCtrl(_panel, wxID_ANY);
	_amount->SetValue(1);
	_amount->SetRange(0, 65535);
	_amount->Bind(wxEVT_SPINCTRL, [&] (wxSpinEvent& ev) { writeToComponent(); });

	_alertLevel = new wxSpinCtrl(_panel, wxID_ANY);
	_alertLevel->SetValue(1);
	_alertLevel->SetRange(1, 5);
	_alertLevel->Bind(wxEVT_SPINCTRL, [&] (wxSpinEvent& ev) { writeToComponent(); });

	// Main vbox
	wxStaticText* label = new wxStaticText(_panel, wxID_ANY, _("AI:"));
	label->SetFont(label->GetFont().Bold());

	_panel->GetSizer()->Add(label, 0, wxBOTTOM, 6);
	_panel->GetSizer()->Add(_targetCombo, 0, wxBOTTOM | wxEXPAND, 6);

	_panel->GetSizer()->Add(new wxStaticText(_panel, wxID_ANY, _("Amount:")), 0, wxBOTTOM, 6);
	_panel->GetSizer()->Add(_amount, 0, wxBOTTOM, 6);

	_panel->GetSizer()->Add(new wxStaticText(_panel, wxID_ANY, _("Minimum Alert Level:")), 0, wxBOTTOM, 6);
	_panel->GetSizer()->Add(_alertLevel, 0, wxBOTTOM, 6);

	// Populate the SpecifierEditCombo with the first specifier
    _targetCombo->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin buttons with the values from the component arguments
	_amount->SetValue(string::convert<int>(component.getArgument(0)));
	_alertLevel->SetValue(string::convert<int>(component.getArgument(1)));
}

// Write to component
void AlertComponentEditor::writeToComponent() const
{
    if (!_active) return; // still under construction

    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _targetCombo->getSpecifier()
    );

	_component->setArgument(0, string::to_string(_amount->GetValue()));
	_component->setArgument(1, string::to_string(_alertLevel->GetValue()));
}

} // namespace ce

} // namespace objectives
