#include "LightTextureChooser.h"

#include "i18n.h"
#include "ishaders.h"
#include "ui/imainframe.h"
#include "igame.h"
#include "texturelib.h"
#include "iregistry.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>

namespace ui
{

LightTextureChooser::LightTextureChooser() :
	wxutil::DialogBase(_("Choose texture"), GlobalMainFrame().getWxTopLevelWindow())
{
	// Create a default panel to this dialog
	wxPanel* mainPanel = new wxPanel(this, wxID_ANY);
	mainPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* dialogVBox = new wxBoxSizer(wxVERTICAL);
	mainPanel->GetSizer()->Add(dialogVBox, 1, wxEXPAND | wxALL, 12);	

	_selector = new ShaderSelector(mainPanel, {}, ShaderSelector::TextureFilter::Lights);

	// Pack in the ShaderSelector and buttons panel
	dialogVBox->Add(_selector, 1, wxEXPAND);

	createButtons(mainPanel, dialogVBox);

	// Set the default size of the window
	FitToScreen(0.6f, 0.6f);
}

// Construct the buttons
void LightTextureChooser::createButtons(wxPanel* mainPanel, wxBoxSizer* dialogVBox)
{
	wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);

	wxButton* okButton = new wxButton(mainPanel, wxID_OK);
	wxButton* cancelButton = new wxButton(mainPanel, wxID_CANCEL);

	buttons->Add(okButton);
	buttons->Add(cancelButton, 0, wxLEFT, 6);

	dialogVBox->Add(buttons, 0, wxALIGN_RIGHT | wxTOP, 6);
}

std::string LightTextureChooser::getSelectedTexture()
{
	// Show all widgets and enter a recursive main loop
	return _selector->getSelection();
}

void LightTextureChooser::setSelectedTexture(const std::string& textureName)
{
    _selector->setSelection(textureName);
}

} // namespace ui
