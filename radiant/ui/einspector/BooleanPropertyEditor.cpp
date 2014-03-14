#include "BooleanPropertyEditor.h"

#include "ientity.h"

#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>

namespace ui
{

// Blank ctor
BooleanPropertyEditor::BooleanPropertyEditor() :
	_checkBox(NULL)
{}

// Constructor. Create the widgets here
BooleanPropertyEditor::BooleanPropertyEditor(wxWindow* parent, Entity* entity,
											 const std::string& name)
: PropertyEditor(entity),
  _checkBox(NULL),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Create the checkbox with correct initial state, and connect up the
	// toggle callback
	_checkBox = new wxCheckBox(parent, wxID_ANY, name);
	_checkBox->SetValue(_entity->getKeyValue(_key) == "1");

	_checkBox->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(BooleanPropertyEditor::_onToggle), NULL, this);

	mainVBox->GetSizer()->Add(_checkBox, 0, wxALIGN_CENTER);
}

void BooleanPropertyEditor::_onToggle(wxCommandEvent& ev)
{
	// Set the key based on the checkbutton state
	setKeyValue(_key, _checkBox->IsChecked() ? "1" : "0");
}

} // namespace
