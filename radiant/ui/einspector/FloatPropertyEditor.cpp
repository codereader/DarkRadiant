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

FloatPropertyEditor::FloatPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _spinCtrl(nullptr),
  _key(key)
{
	// Construct the main widget (will be managed by the base class)
	auto mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Create the HScale and pack into widget
	_spinCtrl = new wxSpinCtrlDouble(mainVBox, wxID_ANY);

	_spinCtrl->SetIncrement(1.0);
	_spinCtrl->SetMinSize(wxSize(75, -1));

	// Set the initial value if the entity has one
	updateFromEntity();

	// Create and pack in the Apply button
	wxButton* applyButton = new wxButton(mainVBox, wxID_APPLY, _("Apply"));
	applyButton->Bind(wxEVT_BUTTON, &FloatPropertyEditor::_onApply, this);

	mainVBox->GetSizer()->Add(_spinCtrl, 0, wxALIGN_CENTER_VERTICAL);
	mainVBox->GetSizer()->Add(applyButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
}

void FloatPropertyEditor::updateFromEntity()
{
	if (_spinCtrl == nullptr) return;

	auto value = string::convert<float>(_entities.getSharedKeyValue(_key->getFullKey(), false), 0);
	
	_spinCtrl->SetValue(value);
}

void FloatPropertyEditor::_onApply(wxCommandEvent& ev)
{
	float value = static_cast<float>(_spinCtrl->GetValue());

    setKeyValueOnSelection(_key->getFullKey(), string::to_string(value));
}

}
