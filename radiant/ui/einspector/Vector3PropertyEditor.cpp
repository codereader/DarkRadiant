#include "Vector3PropertyEditor.h"

#include "i18n.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>

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

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	mainVBox->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	
    // Create the spin buttons 
	_xValue = new wxSpinCtrl(mainVBox, wxID_ANY);
    _yValue = new wxSpinCtrl(mainVBox, wxID_ANY);
	_zValue = new wxSpinCtrl(mainVBox, wxID_ANY);

	_xValue->SetValue(0);
	_yValue->SetValue(0);
	_zValue->SetValue(0);

	_xValue->SetRange(-32767, 32767);
	_yValue->SetRange(-32767, 32767);
	_zValue->SetRange(-32767, 32767);

    // Add the spin buttons to the HBox with labels
	hbox->Add(new wxStaticText(mainVBox, wxID_ANY, _("X: ")));
	hbox->Add(_xValue);
	hbox->Add(new wxStaticText(mainVBox, wxID_ANY, _(" Y: ")));
	hbox->Add(_yValue);
	hbox->Add(new wxStaticText(mainVBox, wxID_ANY, _(" Z: ")));
	hbox->Add(_zValue);

	// Pack edit box into the main widget
	mainVBox->GetSizer()->Add(hbox);

	// Create the apply button
	wxButton* applyButton = new wxButton(mainVBox, wxID_APPLY, _("Apply..."));
	applyButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(Vector3PropertyEditor::_onApply), NULL, this);

	// Populate the spin boxes from the keyvalue
	setWidgetsFromKey(_entity->getKeyValue(name));
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
	using boost::lexical_cast;
	using std::string;

	// Construct a text value out of the vector components
	std::string value =
		lexical_cast<string>(_xValue->GetValue())
		+ " "
		+ lexical_cast<string>(_yValue->GetValue())
		+ " "
		+ lexical_cast<string>(_zValue->GetValue());

	// Set the key on the entity
	setKeyValue(_key, value);
}

}
