#include "AnglePropertyEditor.h"

#include "iuimanager.h"
#include "ientity.h"
#include "string/convert.h"

#include <wx/bmpbuttn.h>
#include <wx/panel.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ui
{

// Constructor
AnglePropertyEditor::AnglePropertyEditor(wxWindow* parent, Entity* entity, const std::string& key)
: PropertyEditor(entity),
  _key(key)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Construct a 3x3 table to contain the directional buttons
	wxGridSizer* grid = new wxGridSizer(3, 3, 3);
	mainVBox->GetSizer()->Add(grid, 0, wxALIGN_CENTER_VERTICAL);

    // Create the buttons
    constructButtons(mainVBox, grid);

	// Register the main widget in the base class
	setMainWidget(mainVBox);
}

wxBitmapButton* AnglePropertyEditor::constructAngleButton(wxPanel* parent, const std::string& icon, int angleValue)
{
	wxBitmapButton* button = new wxBitmapButton(parent, wxID_ANY, 
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + icon));

	button->Connect(wxEVT_BUTTON, wxCommandEventHandler(AnglePropertyEditor::_onButtonClick), NULL, this);

	// Store the angle value in the map for later use
	_buttons[button] = angleValue;

	return button;
}

// Construct the buttons
void AnglePropertyEditor::constructButtons(wxPanel* parent, wxGridSizer* grid)
{
	grid->Add(constructAngleButton(parent, "arrow_nw24.png", 135));
	grid->Add(constructAngleButton(parent, "arrow_n24.png", 90));
	grid->Add(constructAngleButton(parent, "arrow_ne24.png", 45));

	grid->Add(constructAngleButton(parent, "arrow_w24.png", 180));
	grid->Add(new wxStaticText(parent, -1, wxT("")));
	grid->Add(constructAngleButton(parent, "arrow_e24.png", 0));

	grid->Add(constructAngleButton(parent, "arrow_sw24.png", 225));
	grid->Add(constructAngleButton(parent, "arrow_s24.png", 270));
	grid->Add(constructAngleButton(parent, "arrow_se24.png", 315));
}

void AnglePropertyEditor::_onButtonClick(wxCommandEvent& ev)
{
	for (ButtonMap::const_iterator i = _buttons.begin(); i != _buttons.end(); ++i)
	{
		if (i->first->GetId() == ev.GetId())
		{
			setKeyValue(_key, string::to_string(i->second));
			break;
		}
	}
}

} // namespace ui
