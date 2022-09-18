#include "SoundChooser.h"

#include "i18n.h"
#include "isound.h"
#include "ideclmanager.h"
#include "registry/registry.h"

#include "wxutil/dataview/VFSTreePopulator.h"
#include "ui/UserInterfaceModule.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>

#include <sigc++/functors/mem_fun.h>

namespace ui
{

namespace
{
    constexpr const char* const RKEY_WINDOW_STATE = "user/ui/soundChooser/window";
    constexpr const char* const RKEY_LAST_SELECTED_SHADER = "user/ui/soundChooser/lastSelectedShader";
}

// Constructor
SoundChooser::SoundChooser(wxWindow* parent) :
	DialogBase(_("Choose sound"), parent),
    _selector(nullptr)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));
	
    auto* buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    auto* reloadButton = new wxButton(this, wxID_ANY, _("Reload Sounds"));
    reloadButton->Bind(wxEVT_BUTTON, &SoundChooser::_onReloadSounds, this);

    buttonSizer->Prepend(reloadButton, 0, wxRIGHT, 32);
    auto* dblClickHint = new wxStaticText(this, wxID_ANY, 
        _( "Ctrl + Double Click on treeview items for quick play" ), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    auto* grid = new wxFlexGridSizer( 2 );
    grid->AddGrowableCol( 1 );
    grid->Add( dblClickHint, 0, wxALIGN_CENTER_VERTICAL );
    grid->Add( buttonSizer, 0, wxALIGN_RIGHT );

    _selector = new SoundShaderSelector(this);
    _selector->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &SoundChooser::_onItemActivated, this);

	GetSizer()->Add(_selector, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);
	GetSizer()->Add(grid, 0, wxEXPAND | wxALL, 12);

    _windowPosition.initialise(this, RKEY_WINDOW_STATE, 0.5f, 0.7f);

    // Load the shaders
    loadSoundShaders();
}

void SoundChooser::loadSoundShaders()
{
    _selector->Populate();
}

std::string SoundChooser::getSelectedShader() const
{
    return _selector->GetSelectedDeclName();
}

void SoundChooser::setSelectedShader(const std::string& shader)
{
    _selector->SetSelectedDeclName(shader);
}

void SoundChooser::_onItemActivated(wxDataViewEvent& ev)
{
    auto selectedItem = _selector->GetSelectedDeclName();

    if (!selectedItem.empty() && !wxGetKeyState(WXK_CONTROL))
    {
        // simple double click closes modal, ctrl+dblclk plays sound
        EndModal(wxID_OK);
    }

    ev.Skip();
}

int SoundChooser::ShowModal()
{
    _windowPosition.applyPosition();

    _shadersReloaded = GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::SoundShader)
        .connect(sigc::mem_fun(this, &SoundChooser::onShadersReloaded));

	int returnCode = DialogBase::ShowModal();

    _windowPosition.saveToPath(RKEY_WINDOW_STATE);
    _shadersReloaded.disconnect();

	return returnCode;
}

void SoundChooser::_onReloadSounds(wxCommandEvent& ev)
{
    _selector->SetSelectedDeclName({});

    // Send the command to the SoundManager
    // After parsing it will fire the sounds reloaded signal => onShadersReloaded()
    GlobalCommandSystem().executeCommand("ReloadSounds");
}

void SoundChooser::onShadersReloaded()
{
    // This signal is fired by the SoundManager, possibly from a non-UI thread
    GetUserInterfaceModule().dispatch([this]()
    {
        loadSoundShaders();
    });
}

std::string SoundChooser::chooseResource(const std::string& shaderToPreselect)
{
    auto preselected = !shaderToPreselect.empty() ? shaderToPreselect :
        registry::getValue<std::string>(RKEY_LAST_SELECTED_SHADER);

	if (!preselected.empty())
	{
		setSelectedShader(preselected);
	}

	std::string selectedShader;

	if (ShowModal() == wxID_OK)
	{
		selectedShader = getSelectedShader();

        if (!selectedShader.empty())
        {
            registry::setValue(RKEY_LAST_SELECTED_SHADER, selectedShader);
        }
	}

	return selectedShader;
}

void SoundChooser::destroyDialog()
{
	Destroy();
}

} // namespace
