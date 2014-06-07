#include "KnockoutComponentEditor.h"
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
KnockoutComponentEditor::RegHelper KnockoutComponentEditor::regHelper;

// Constructor
KnockoutComponentEditor::KnockoutComponentEditor(wxWindow* parent, Component& component) :
	_component(&component),
	_targetCombo(new SpecifierEditCombo(parent, SpecifierType::SET_STANDARD_AI()))
{
	_amount = new wxSpinCtrl(parent, wxID_ANY);
	_amount->SetValue(1);
	_amount->SetRange(0, 65535);

	wxStaticText* label = new wxStaticText(parent, wxID_ANY, _("Knockout target:"));
	label->SetFont(label->GetFont().Bold());

	_panel->GetSizer()->Add(label, 0, wxBOTTOM, 6);
	_panel->GetSizer()->Add(_targetCombo, 0, wxBOTTOM | wxEXPAND, 6);

	_panel->GetSizer()->Add(new wxStaticText(parent, wxID_ANY, _("Amount:")), 0, wxBOTTOM, 6);
	_panel->GetSizer()->Add(_amount, 0, wxBOTTOM | wxEXPAND, 6);

    // Populate the SpecifierEditCombo with the first specifier
    _targetCombo->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin button with the value from the first component argument
	_amount->SetValue(string::convert<int>(component.getArgument(0)));
}

// Write to component
void KnockoutComponentEditor::writeToComponent() const
{
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _targetCombo->getSpecifier()
    );

	_component->setArgument(0, string::to_string(_amount->GetValue()));
}

}

}
