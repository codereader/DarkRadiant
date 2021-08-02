#include "GameConnectionDialog.h"
#include "GameConnection.h"

#include "i18n.h"
#include "imainframe.h"
#include "idialogmanager.h"

#include <wx/activityindicator.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/sizer.h>


namespace
{
    const char* GameConnectionDialog_TITLE = N_("Game connection");
}


namespace gameconn
{

void showError(const std::string& text)
{
    auto dlg = GlobalDialogManager().createMessageBox(
        _("Game connection error"), text, ui::IDialog::MESSAGE_ERROR
    );
    if (dlg)
        dlg->run();
}

GameConnectionDialog& GameConnectionDialog::Instance()
{
    static std::unique_ptr<GameConnectionDialog> _instance;

    if (!_instance) {
        _instance.reset(new GameConnectionDialog());

        // Pre-destruction cleanup
        GlobalMainFrame().signal_MainFrameShuttingDown().connect([]() {
            if (_instance->IsShownOnScreen())
                _instance->Hide();
            _instance->SendDestroyEvent();
            _instance.reset();
        });
    }

    return *_instance;
}

void GameConnectionDialog::toggleDialog(const cmd::ArgumentList& args)
{
    // Toggle the instance
    Instance().ToggleVisibility();
}

GameConnectionDialog::~GameConnectionDialog()
{
    _updateOnStatusChangeSignal.disconnect();
}

GameConnectionDialog::GameConnectionDialog() :
    wxutil::TransientWindow(_(GameConnectionDialog_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true)
{
    wxPanel* panel = loadNamedPanel(this, "GameConnectionMainPanel");

    //could not find activity indicator in wxFormBuilder
    wxActivityIndicator* ConnectedActivityIndicator = new wxActivityIndicator(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxT("OMG"));
    replaceControl(findNamedObject<wxWindow>(this, "ConnectedActivityIndicator"), ConnectedActivityIndicator);

    //don't want to call findNamedObject every time, risking a typo
    _connectedCheckbox               = findNamedObject<wxCheckBox>(this, "ConnectedCheckbox");
    _restartGameButton               = findNamedObject<wxButton  >(this, "RestartGameButton");
    _cameraLoadFromGameButton        = findNamedObject<wxButton  >(this, "CameraLoadFromGameButton");
    _cameraSendToGameCheckbox        = findNamedObject<wxCheckBox>(this, "CameraSendToGameCheckbox");
    _mapFileReloadNowButton          = findNamedObject<wxButton  >(this, "MapFileReloadNowButton");
    _mapFileReloadOnSaveCheckbox     = findNamedObject<wxCheckBox>(this, "MapFileReloadOnSaveCheckbox");
    _hotReloadUpdateNowButton        = findNamedObject<wxButton  >(this, "HotReloadUpdateNowButton");
    _hotReloadUpdateOnChangeCheckbox = findNamedObject<wxCheckBox>(this, "HotReloadUpdateOnChangeCheckbox");
    _respawnSelectedButton           = findNamedObject<wxButton  >(this, "RespawnSelectedButton");
    _pauseGameButton                 = findNamedObject<wxButton  >(this, "PauseGameButton");

    //initially we are not connected, so disable most of GUI
    updateConnectedStatus();
    //update GUI when anything changes in connection
    _updateOnStatusChangeSignal = Impl().signal_StatusChanged.connect([this](int) {
        GameConnectionDialog::updateConnectedStatus();
    });

    //===================================
    //         EVENT HANDLERS
    //===================================

    _connectedCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev) {
        if (_connectedCheckbox->IsChecked()) {
            bool success = Impl().connect();
            if (!success)
                showError("Failed to connect to game.\nMaybe try 'Restart game' button?");
        }
        else
            Impl().disconnect(true);

        updateConnectedStatus();
    });
    _restartGameButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev) {
        //TODO
    });

    _cameraLoadFromGameButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev) {
        Impl().backSyncCamera();
    });
    _cameraSendToGameCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev) {
        bool makeEnabled = _cameraSendToGameCheckbox->IsChecked();
        Impl().setCameraSyncEnabled(makeEnabled);
    });

    _mapFileReloadNowButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev) {
        Impl().reloadMap();
    });
    _mapFileReloadOnSaveCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev) {
        bool makeEnabled = _mapFileReloadOnSaveCheckbox->IsChecked();
        Impl().setAutoReloadMapEnabled(makeEnabled);
    });

    _hotReloadUpdateNowButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev) {
        Impl().doUpdateMap();
    });
    _hotReloadUpdateOnChangeCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev) {
        bool makeEnabled = _hotReloadUpdateOnChangeCheckbox->IsChecked();
        Impl().setUpdateMapAlways(makeEnabled);
    });

    _respawnSelectedButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev) {
        Impl().respawnSelectedEntities();
    });
    _pauseGameButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev) {
        Impl().togglePauseGame();
    });
}

void GameConnectionDialog::_preShow()
{
    TransientWindow::_preShow();
}

void GameConnectionDialog::_preHide()
{
    TransientWindow::_preHide();
}

GameConnection& GameConnectionDialog::Impl()
{
    static module::InstanceReference<GameConnection> _reference("GameConnection");
    return _reference;
}

void GameConnectionDialog::updateConnectedStatus()
{
    bool connected = Impl().isAlive();
    bool updateMapMode = Impl().isUpdateMapObserverEnabled();

    _connectedCheckbox->SetValue(connected);

    _cameraLoadFromGameButton           ->Enable(connected);
    _cameraSendToGameCheckbox           ->Enable(connected);
    _mapFileReloadNowButton             ->Enable(connected);
    _mapFileReloadOnSaveCheckbox        ->Enable(connected);
    _hotReloadUpdateNowButton           ->Enable(connected && updateMapMode);
    _hotReloadUpdateOnChangeCheckbox    ->Enable(connected && updateMapMode);
    _respawnSelectedButton              ->Enable(connected);
    _pauseGameButton                    ->Enable(connected);

    if (!connected) {
        _cameraSendToGameCheckbox           ->SetValue(false);
        _mapFileReloadOnSaveCheckbox        ->SetValue(false);
        _hotReloadUpdateOnChangeCheckbox    ->SetValue(false);
    }
    if (!updateMapMode) {
        _hotReloadUpdateOnChangeCheckbox    ->SetValue(false);
    }
}

}
