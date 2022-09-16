#include "MaterialChooser.h"

#include "i18n.h"
#include "ishaders.h"
#include "texturelib.h"

#include <wx/button.h>
#include <wx/textctrl.h>

#include "../materials/MaterialSelector.h"

namespace ui
{
	namespace
	{
		const char* const LABEL_TITLE = N_("Choose Material");
		const std::string RKEY_WINDOW_STATE = "user/ui/textures/materialChooser/window";
	}

// Construct the dialog
MaterialChooser::MaterialChooser(wxWindow* parent, MaterialSelector::TextureFilter filter, wxTextCtrl* targetEntry) :
	wxutil::DialogBase(_(LABEL_TITLE), parent),
	_targetEntry(targetEntry),
	_selector(nullptr)
{
	// Create a default panel to this dialog
	wxPanel* mainPanel = new wxPanel(this, wxID_ANY);
	mainPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* dialogVBox = new wxBoxSizer(wxVERTICAL);
	mainPanel->GetSizer()->Add(dialogVBox, 1, wxEXPAND | wxALL, 12);

	_selector = new MaterialSelector(mainPanel,
        std::bind(&MaterialChooser::shaderSelectionChanged, this), filter);

	if (_targetEntry != nullptr)
	{
		_initialShader = _targetEntry->GetValue();

		// Set the cursor of the tree view to the currently selected shader
		_selector->SetSelectedDeclName(_initialShader);
	}

    _selector->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &MaterialChooser::_onItemActivated, this);

	// Pack in the MaterialSelector and buttons panel
	dialogVBox->Add(_selector, 1, wxEXPAND);

	createButtons(mainPanel, dialogVBox);

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

std::string MaterialChooser::getSelectedTexture()
{
    return _selector->GetSelectedDeclName();
}

void MaterialChooser::setSelectedTexture(const std::string& textureName)
{
    _selector->SetSelectedDeclName(textureName);
}

void MaterialChooser::shutdown()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
}

void MaterialChooser::_onItemActivated(wxDataViewEvent& ev)
{
    if (!_selector->GetSelectedDeclName().empty())
    {
        if (_targetEntry)
        {
            _targetEntry->SetValue(_selector->GetSelectedDeclName());
        }

        shutdown();
        EndModal(wxID_OK);
    }
}

// Construct the buttons
void MaterialChooser::createButtons(wxPanel* mainPanel, wxBoxSizer* dialogVBox)
{
	wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);

	wxButton* okButton = new wxButton(mainPanel, wxID_OK);
	wxButton* cancelButton = new wxButton(mainPanel, wxID_CANCEL);

	okButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(MaterialChooser::callbackOK), NULL, this);
	cancelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(MaterialChooser::callbackCancel), NULL, this);

	buttons->Add(okButton);
	buttons->Add(cancelButton, 0, wxLEFT, 6);

	dialogVBox->Add(buttons, 0, wxALIGN_RIGHT | wxTOP, 6);
}

void MaterialChooser::shaderSelectionChanged()
{
	if (_targetEntry)
	{
		_targetEntry->SetValue(_selector->GetSelectedDeclName());
	}

	// Propagate the call up to the client (e.g. SurfaceInspector)
    _shaderChangedSignal.emit();
}

void MaterialChooser::revertShader()
{
	// Revert the shadername to the value it had at dialog startup
	if (_targetEntry)
	{
		_targetEntry->SetValue(_initialShader);

		// Propagate the call up to the client (e.g. SurfaceInspector)
        _shaderChangedSignal.emit();
	}
}

void MaterialChooser::callbackCancel(wxCommandEvent& ev)
{
	// Revert the shadername to the value it had at dialog startup
	revertShader();
	shutdown();

	ev.Skip();
}

void MaterialChooser::callbackOK(wxCommandEvent& ev)
{
	if (_targetEntry)
	{
		_targetEntry->SetValue(_selector->GetSelectedDeclName());
	}

	shutdown();

	ev.Skip(); // let the default handler end the modal session
}

} // namespace ui
