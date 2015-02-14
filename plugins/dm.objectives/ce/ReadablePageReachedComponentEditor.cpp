#include "ReadablePageReachedComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "i18n.h"
#include "string/convert.h"

#include <wx/stattext.h>
#include <wx/spinctrl.h>

namespace objectives
{

namespace ce
{

// Registration helper, will register this editor in the factory
ReadablePageReachedComponentEditor::RegHelper ReadablePageReachedComponentEditor::regHelper;

// Constructor
ReadablePageReachedComponentEditor::ReadablePageReachedComponentEditor(wxWindow* parent, Component& component) :
	ComponentEditorBase(parent),
	_component(&component),
	_readableSpec(new SpecifierEditCombo(_panel, getChangeCallback(), SpecifierType::SET_READABLE()))
{
	_pageNum = new wxSpinCtrl(_panel, wxID_ANY);
	_pageNum->SetValue(1);
	_pageNum->SetRange(0, 65535);
	_pageNum->Bind(wxEVT_SPINCTRL, [&] (wxSpinEvent& ev) { writeToComponent(); });

	wxStaticText* label = new wxStaticText(_panel, wxID_ANY, _("Readable:"));
	label->SetFont(label->GetFont().Bold());

	_panel->GetSizer()->Add(label, 0, wxBOTTOM | wxEXPAND, 6);
	_panel->GetSizer()->Add(_readableSpec, 0, wxBOTTOM | wxEXPAND, 6);

	_panel->GetSizer()->Add(new wxStaticText(_panel, wxID_ANY, _("Page Number:")), 0, wxBOTTOM, 6);
	_panel->GetSizer()->Add(_pageNum, 0, wxBOTTOM | wxEXPAND, 6);

    // Populate the SpecifierEditCombo with the first specifier
    _readableSpec->setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );

	// Initialise the spin button with the value from the first component argument
	_pageNum->SetValue(string::convert<int>(component.getArgument(0)));
}

// Write to component
void ReadablePageReachedComponentEditor::writeToComponent() const
{
    if (!_active) return; // still under construction

    assert(_component);

    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _readableSpec->getSpecifier()
    );

	_component->setArgument(0, string::to_string(_pageNum->GetValue()));
}

} // namespace ce

} // namespace objectives
