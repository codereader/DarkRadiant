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

static const int RANGE_MAX = 32767;
static const int RANGE_MIN = -32767;

wxSpinCtrl* makeSpinCtrl(wxPanel* parent)
{
    wxSpinCtrl* ctrl = new wxSpinCtrl(parent, wxID_ANY);

    // Set an appropriate minimum size based on the expected contents
    static const wxSize minSize = ctrl->GetSizeFromTextSize(
        ctrl->GetTextExtent(std::to_string(RANGE_MIN)).GetWidth()
    );
	ctrl->SetMinSize(minSize);

    // Set value and range
    ctrl->SetValue(0);
    ctrl->SetRange(RANGE_MIN, RANGE_MAX);

    return ctrl;
}

// Constructor. Create the widgets here
Vector3PropertyEditor::Vector3PropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
    // Construct the main widget (will be managed by the base class)
    wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
    mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

    // Register the main widget in the base class
    setMainWidget(mainVBox);

    // Create the spin buttons 
    _xValue = makeSpinCtrl(mainVBox);
    _yValue = makeSpinCtrl(mainVBox);
    _zValue = makeSpinCtrl(mainVBox);

    // Add the spin buttons to the HBox with labels
    mainVBox->GetSizer()->Add(new wxStaticText(mainVBox, wxID_ANY, _("X: ")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    mainVBox->GetSizer()->Add(_xValue, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    mainVBox->GetSizer()->Add(new wxStaticText(mainVBox, wxID_ANY, _(" Y: ")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    mainVBox->GetSizer()->Add(_yValue, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    mainVBox->GetSizer()->Add(new wxStaticText(mainVBox, wxID_ANY, _(" Z: ")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    mainVBox->GetSizer()->Add(_zValue, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);

    // Create the apply button
    wxButton* applyButton = new wxButton(mainVBox, wxID_APPLY, _("Apply"));
    applyButton->Bind(wxEVT_BUTTON, &Vector3PropertyEditor::_onApply, this);

    mainVBox->GetSizer()->Add(applyButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);

    // Populate the spin boxes from the keyvalue
    updateFromEntity();
}

void Vector3PropertyEditor::updateFromEntity()
{
	setWidgetsFromKey(_entities.getSharedKeyValue(_key->getFullKey(), false));
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

    // Set the widgets
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
    setKeyValueOnSelection(_key->getFullKey(), value);
}

}
