#include "ShaderChooser.h"

#include "i18n.h"
#include "iregistry.h"
#include "ishaders.h"
#include "texturelib.h"

#include <wx/button.h>
#include <wx/textctrl.h>

namespace ui
{
	namespace
	{
		const char* const LABEL_TITLE = N_("Choose Shader");
		const std::string SHADER_PREFIXES = "textures";
		const std::string RKEY_WINDOW_STATE = "user/ui/textures/shaderChooser/window";
	}

// Construct the dialog
ShaderChooser::ShaderChooser(wxWindow* parent, wxTextCtrl* targetEntry) :
	wxutil::DialogBase(_(LABEL_TITLE), parent),
	_targetEntry(targetEntry),
	_selector(NULL)
{
	// Create a default panel to this dialog
	wxPanel* mainPanel = new wxPanel(this, wxID_ANY);
	mainPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* dialogVBox = new wxBoxSizer(wxVERTICAL);
	mainPanel->GetSizer()->Add(dialogVBox, 1, wxEXPAND | wxALL, 12);

	_selector = new ShaderSelector(mainPanel, this, SHADER_PREFIXES);

	if (_targetEntry != NULL)
	{
		_initialShader = targetEntry->GetValue();

		// Set the cursor of the tree view to the currently selected shader
		_selector->setSelection(_initialShader);
	}

	// Pack in the ShaderSelector and buttons panel
	dialogVBox->Add(_selector, 1, wxEXPAND);

	createButtons(mainPanel, dialogVBox);

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

void ShaderChooser::shutdown()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
}

// Construct the buttons
void ShaderChooser::createButtons(wxPanel* mainPanel, wxBoxSizer* dialogVBox)
{
	wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);

	wxButton* okButton = new wxButton(mainPanel, wxID_OK);
	wxButton* cancelButton = new wxButton(mainPanel, wxID_CANCEL);

	okButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ShaderChooser::callbackOK), NULL, this);
	cancelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ShaderChooser::callbackCancel), NULL, this);

	buttons->Add(okButton);
	buttons->Add(cancelButton, 0, wxLEFT, 6);

	dialogVBox->Add(buttons, 0, wxALIGN_RIGHT | wxTOP, 6);
}

void ShaderChooser::shaderSelectionChanged(const std::string& shaderName,
										   wxutil::TreeModel& listStore)
{
	if (_targetEntry != NULL)
	{
		_targetEntry->SetValue(_selector->getSelection());
	}

	// Propagate the call up to the client (e.g. SurfaceInspector)
    _shaderChangedSignal.emit();

	// Get the shader, and its image map if possible
	MaterialPtr shader = _selector->getSelectedShader();
	// Pass the call to the static member
	ShaderSelector::displayShaderInfo(shader, listStore);
}

void ShaderChooser::revertShader()
{
	// Revert the shadername to the value it had at dialog startup
	if (_targetEntry != NULL)
	{
		_targetEntry->SetValue(_initialShader);

		// Propagate the call up to the client (e.g. SurfaceInspector)
        _shaderChangedSignal.emit();
	}
}

void ShaderChooser::callbackCancel(wxCommandEvent& ev)
{
	// Revert the shadername to the value it had at dialog startup
	revertShader();
	shutdown();

	ev.Skip();
}

void ShaderChooser::callbackOK(wxCommandEvent& ev)
{
	if (_targetEntry != NULL)
	{
		_targetEntry->SetValue(_selector->getSelection());
	}

	shutdown();

	ev.Skip(); // let the default handler end the modal session
}

} // namespace ui
