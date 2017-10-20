#include "FloatPropertyEditor.h"

#include "ientity.h"
#include "i18n.h"

#include "itextstream.h"
#include <vector>

#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>

#include "string/split.h"
#include "string/convert.h"

namespace ui
{

FloatPropertyEditor::FloatPropertyEditor() :
	_spinCtrl(nullptr)
{}

// Main constructor
FloatPropertyEditor::FloatPropertyEditor(wxWindow* parent, Entity* entity,
										 const std::string& key,
										 const std::string& options)
: PropertyEditor(entity),
  _spinCtrl(nullptr),
  _key(key)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Split the options string to get min and max values
	std::vector<std::string> values;
	string::split(values, options, ",");
	if (values.size() != 2)
		return;

	// Attempt to cast to min and max floats
	float min, max;
	try
	{
		min = std::stof(values[0]);
		max = std::stof(values[1]);
	}
	catch (std::invalid_argument&)
	{
		rError() << "[radiant] FloatPropertyEditor failed to parse options string "
			<< "\"" << options << "\"" << std::endl;
		return;
	}

	// Create the HScale and pack into widget
	_spinCtrl = new wxSpinCtrlDouble(parent, wxID_ANY);

	_spinCtrl->SetIncrement(1.0);
	_spinCtrl->SetRange(min, max);
	_spinCtrl->SetMinSize(wxSize(75, -1));

	// Set the initial value if the entity has one
	updateFromEntity();

	// Create and pack in the Apply button
	wxButton* applyButton = new wxButton(mainVBox, wxID_APPLY, _("Apply..."));
	applyButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FloatPropertyEditor::_onApply), NULL, this);

	mainVBox->GetSizer()->Add(_spinCtrl, 0, wxALIGN_CENTER_VERTICAL);
	mainVBox->GetSizer()->Add(applyButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
}

void FloatPropertyEditor::updateFromEntity()
{
	if (_spinCtrl == nullptr) return;

	float value = string::convert<float>(_entity->getKeyValue(_key), 0);
	
	_spinCtrl->SetValue(value);
}

void FloatPropertyEditor::_onApply(wxCommandEvent& ev)
{
	float value = static_cast<float>(_spinCtrl->GetValue());

	setKeyValue(_key, string::to_string(value));
}

}
