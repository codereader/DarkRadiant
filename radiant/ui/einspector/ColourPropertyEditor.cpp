#include "ColourPropertyEditor.h"

#include "ientity.h"

#include <wx/clrpicker.h>
#include <wx/sizer.h>
#include <wx/panel.h>

#include <fmt/format.h>
#include <sstream>

namespace ui
{

// Main ctor
ColourPropertyEditor::ColourPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
: PropertyEditor(entities),
  _key(key)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Create the colour button
    _colorButton = new wxColourPickerCtrl(mainVBox, wxID_ANY, 
                                          wxColour(0, 0, 0), wxDefaultPosition, 
                                          wxDefaultSize, wxCLRP_USE_TEXTCTRL);

	_colorButton->Connect(wxEVT_COLOURPICKER_CHANGED, 
		wxColourPickerEventHandler(ColourPropertyEditor::_onColorSet), NULL, this);

	mainVBox->GetSizer()->Add(_colorButton, 1, wxEXPAND | wxALL, 15);

	updateFromEntity();
}

void ColourPropertyEditor::updateFromEntity()
{
	// Set colour button's colour, also take inherited values into account
	setColourButton(getKeyValueFromSelection(_key->getFullKey()));
}

// Set displayed colour from the keyvalue
void ColourPropertyEditor::setColourButton(const std::string& val)
{
	float r = 0.0, g = 0.0, b = 0.0;
	std::stringstream str(val);

	// Stream the whitespace-separated string into floats
	str >> r;
	str >> g;
	str >> b;

	_colorButton->SetColour(wxColour(r*255, g*255, b*255));
}

// Get selected colour
std::string ColourPropertyEditor::getSelectedColour()
{
	// Get colour from the button
	wxColour col = _colorButton->GetColour();

	// Format the string value appropriately.
	return fmt::format("{0:.3f} {1:.3f} {2:.3f}",
			(col.Red() / 255.0f),
			(col.Green() / 255.0f),
			(col.Blue() / 255.0f));
}

void ColourPropertyEditor::_onColorSet(wxColourPickerEvent& ev)
{
	// Set the new keyvalue on the entity
    setKeyValueOnSelection(_key->getFullKey(), getSelectedColour());
}


} // namespace ui

