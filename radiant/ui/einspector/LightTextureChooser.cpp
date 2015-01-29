#include "LightTextureChooser.h"

#include "i18n.h"
#include "ishaders.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "igame.h"
#include "igroupdialog.h"
#include "texturelib.h"
#include "iregistry.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>

namespace ui
{

namespace
{
	const char* const LIGHT_PREFIX_XPATH = "/light/texture//prefix";

	/** greebo: Loads the prefixes from the registry and creates a
	 * 			comma-separated list string
	 */
	inline std::string getPrefixList()
	{
		std::string prefixes;

		// Get the list of light texture prefixes from the registry
		xml::NodeList prefList = GlobalGameManager().currentGame()->getLocalXPath(LIGHT_PREFIX_XPATH);

		// Copy the Node contents into the prefix vector
		for (xml::NodeList::iterator i = prefList.begin();
			 i != prefList.end();
			 ++i)
		{
			prefixes += (prefixes.empty()) ? "" : ",";
			prefixes += i->getContent();
		}

		return prefixes;
	}
}

// Construct the dialog
LightTextureChooser::LightTextureChooser() :
	wxutil::DialogBase(_("Choose texture"), GlobalMainFrame().getWxTopLevelWindow())
{
	// Create a default panel to this dialog
	wxPanel* mainPanel = new wxPanel(this, wxID_ANY);
	mainPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* dialogVBox = new wxBoxSizer(wxVERTICAL);
	mainPanel->GetSizer()->Add(dialogVBox, 1, wxEXPAND | wxALL, 12);	

	_selector = new ShaderSelector(mainPanel, this, getPrefixList(), true); // true >> render a light texture

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

void LightTextureChooser::shaderSelectionChanged(const std::string& shaderName,
	wxutil::TreeModel& listStore)
{
	// Get the shader, and its image map if possible
	MaterialPtr shader = _selector->getSelectedShader();
	// Pass the call to the static member light shader info
	ShaderSelector::displayLightShaderInfo(shader, listStore);
}

} // namespace ui
