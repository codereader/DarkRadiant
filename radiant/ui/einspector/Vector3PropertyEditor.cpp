#include "Vector3PropertyEditor.h"

#include "i18n.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include "string/convert.h"
#include <sstream>
#include <vector>

namespace ui
{

// Blank ctor

Vector3PropertyEditor::Vector3PropertyEditor() {}

// Constructor. Create the widgets here
Vector3PropertyEditor::Vector3PropertyEditor(wxWindow* parent, Entity* entity,
											 const std::string& name)
: PropertyEditor(entity),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Register the main widget in the base class
	setMainWidget(mainVBox);

    // Create the spin buttons 
	_xValue = new wxSpinCtrl(mainVBox, wxID_ANY);
    _yValue = new wxSpinCtrl(mainVBox, wxID_ANY);
	_zValue = new wxSpinCtrl(mainVBox, wxID_ANY);

	_xValue->SetMinSize(wxSize(75, -1));
	_yValue->SetMinSize(wxSize(75, -1));
	_zValue->SetMinSize(wxSize(75, -1));
	
	_xValue->SetValue(0);
	_yValue->SetValue(0);
	_zValue->SetValue(0);

	_xValue->SetRange(-32767, 32767);
	_yValue->SetRange(-32767, 32767);
	_zValue->SetRange(-32767, 32767);

    // Add the spin buttons to the HBox with labels
	mainVBox->GetSizer()->Add(new wxStaticText(mainVBox, wxID_ANY, _("X: ")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
	mainVBox->GetSizer()->Add(_xValue, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
	mainVBox->GetSizer()->Add(new wxStaticText(mainVBox, wxID_ANY, _(" Y: ")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
	mainVBox->GetSizer()->Add(_yValue, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
	mainVBox->GetSizer()->Add(new wxStaticText(mainVBox, wxID_ANY, _(" Z: ")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
	mainVBox->GetSizer()->Add(_zValue, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);

	// Create the apply button
	wxButton* applyButton = new wxButton(mainVBox, wxID_APPLY, _("Apply..."));
	applyButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(Vector3PropertyEditor::_onApply), NULL, this);

	mainVBox->GetSizer()->Add(applyButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);

	// Populate the spin boxes from the keyvalue
	updateFromEntity();
}

void Vector3PropertyEditor::updateFromEntity()
{
	setWidgetsFromKey(_entity->getKeyValue(_key));
}

void Vector3PropertyEditor::setWidgetsFromKey(const std::string& val)
{
    // Stream the given string into a temporary buffer to compile a vector
    // of 3 components (separated by spaces in the input).
    std::stringstream stream(val);
    std::vector<float> values;
    float buf;

    while (stream >> buf)
        values.push_back(buf);

    // Set the Gtk widgets
    if (values.size() == 3)
	{
		_xValue->SetValue(values[0]);
		_yValue->SetValue(values[1]);
		_zValue->SetValue(values[2]);
    }
}

void Vector3PropertyEditor::_onApply(wxCommandEvent& ev)
{
	// Construct a text value out of the vector components
	std::string value =
		string::to_string(_xValue->GetValue())
		+ " "
		+ string::to_string(_yValue->GetValue())
		+ " "
		+ string::to_string(_zValue->GetValue());

	// Set the key on the entity
	setKeyValue(_key, value);
}

}
