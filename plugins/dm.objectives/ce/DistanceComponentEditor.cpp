#include "DistanceComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "string/convert.h"

#include "i18n.h"

#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>

namespace objectives 
{

namespace ce
{

// Registration helper, will register this editor in the factory
DistanceComponentEditor::RegHelper DistanceComponentEditor::regHelper;

// Constructor
DistanceComponentEditor::DistanceComponentEditor(wxWindow* parent, Component& component) :
	ComponentEditorBase(parent),
	_component(&component),
	_entity(new wxTextCtrl(_panel, wxID_ANY)),
	_location(new wxTextCtrl(_panel, wxID_ANY))
{
	_entity->Bind(wxEVT_TEXT, [&] (wxCommandEvent& ev) { writeToComponent(); });
	_location->Bind(wxEVT_TEXT, [&] (wxCommandEvent& ev) { writeToComponent(); });

	// Allow for one digit of the distance, everything below this step size is insane
	_distance = new wxSpinCtrl(_panel, wxID_ANY);
	_distance->SetValue(1);
	_distance->SetRange(0, 132000);
	_distance->SetMinClientSize(wxSize(_distance->GetCharWidth()*9, -1));
	_distance->Bind(wxEVT_SPINCTRL, [&] (wxSpinEvent& ev) { writeToComponent(); });

	_interval = new wxSpinCtrlDouble(_panel, wxID_ANY);
	_interval->SetValue(1);
	_interval->SetRange(0, 65535);
	_interval->SetIncrement(0.1);
	_interval->SetDigits(1);
	_interval->SetMinClientSize(wxSize(_interval->GetCharWidth()*9, -1));
	_interval->Bind(wxEVT_SPINCTRLDOUBLE, [&] (wxSpinDoubleEvent& ev) { writeToComponent(); });

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	_panel->GetSizer()->Add(hbox, 0, wxBOTTOM | wxEXPAND, 6);

	hbox->Add(new wxStaticText(_panel, wxID_ANY, _("Entity:")), 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
	hbox->Add(_entity, 1, wxEXPAND);

	wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
	_panel->GetSizer()->Add(hbox2, 0, wxBOTTOM | wxEXPAND, 6);

	hbox2->Add(new wxStaticText(_panel, wxID_ANY, _("Location Entity:")), 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
	hbox2->Add(_location, 1, wxEXPAND);

	wxBoxSizer* hbox3 = new wxBoxSizer(wxHORIZONTAL);
	_panel->GetSizer()->Add(hbox3, 0, wxBOTTOM | wxEXPAND, 6);

	hbox3->Add(new wxStaticText(_panel, wxID_ANY, _("Distance:")), 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
	hbox3->Add(_distance, 0);

	wxBoxSizer* hbox4 = new wxBoxSizer(wxHORIZONTAL);
	_panel->GetSizer()->Add(hbox4, 0, wxBOTTOM | wxEXPAND, 6);

	hbox4->Add(new wxStaticText(_panel, wxID_ANY, _("Clock interval:")), 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
	hbox4->Add(_interval, 0);
	hbox4->Add(new wxStaticText(_panel, wxID_ANY, _("seconds:")), 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);

	// Load the initial values from the component arguments
	_entity->SetValue(component.getArgument(0));
	_location->SetValue(component.getArgument(1));

	_distance->SetValue(string::convert<int>(component.getArgument(2)));
	float interval = component.getClockInterval();
	_interval->SetValue(interval >= 0 ? interval : 1.0);
}

// Write to component
void DistanceComponentEditor::writeToComponent() const
{
    if (!_active) return; // still under construction

    assert(_component);

	_component->setArgument(0, _entity->GetValue().ToStdString());
	_component->setArgument(1, _location->GetValue().ToStdString());
	_component->setArgument(2, string::to_string(_distance->GetValue()));
	_component->setClockInterval(static_cast<float>(_interval->GetValue()));
}

} // namespace ce

} // namespace objectives
