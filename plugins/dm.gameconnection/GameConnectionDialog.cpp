#include "GameConnectionDialog.h"
#include "GameConnection.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "ui/idialogmanager.h"

#if HAVE_ACTIVITYINDICATOR
    #include <wx/activityindicator.h>
#else
    #include <wx/statbmp.h>
#endif
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

#if HAVE_ACTIVITYINDICATOR
    //could not find activity indicator in wxFormBuilder
    _connectedActivityIndicator = new wxActivityIndicator(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxT("ConnectedActivityIndicator"));
    replaceControl(findNamedObject<wxWindow>(this, "ConnectedActivityIndicator"), _connectedActivityIndicator);
    _connectedActivityIndicator->Start();
#else
    _connectedActivityIndicator = findNamedObject<wxStaticBitmap>(this, "ConnectedActivityIndicator");
    panel;  //fix warning
#endif
    SetMinSize(panel->GetEffectiveMinSize());
    Layout();

    //don't want to call findNamedObject every time, risking a typo
    _connectedCheckbox               = findNamedObject<wxCheckBox>(this, "ConnectedCheckbox");
    _restartGameButton               = findNamedObject<wxButton  >(this, "RestartGameButton");
    _dmapCheckbox                    = findNamedObject<wxCheckBox>(this, "DmapCheckbox");
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
        bool dmap = _dmapCheckbox->IsChecked();
        Impl().restartGame(dmap);
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
        Impl().setAlwaysUpdateMapEnabled(makeEnabled);
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
    bool restarting = Impl().isGameRestarting();
    bool updateMapMode = Impl().isUpdateMapObserverEnabled();

    _connectedCheckbox->SetValue(connected);

    if (restarting)
        _connectedActivityIndicator->Show();
    else
        _connectedActivityIndicator->Hide();

    _cameraLoadFromGameButton           ->Enable(connected && !restarting);
    _cameraSendToGameCheckbox           ->Enable(connected && !restarting);
    _mapFileReloadNowButton             ->Enable(connected && !restarting);
    _mapFileReloadOnSaveCheckbox        ->Enable(connected && !restarting);
    _hotReloadUpdateNowButton           ->Enable(connected && !restarting && updateMapMode);
    _hotReloadUpdateOnChangeCheckbox    ->Enable(connected && !restarting && updateMapMode);
    _respawnSelectedButton              ->Enable(connected && !restarting);
    _pauseGameButton                    ->Enable(connected && !restarting);

    _cameraSendToGameCheckbox           ->SetValue(Impl().isCameraSyncEnabled());
    _mapFileReloadOnSaveCheckbox        ->SetValue(Impl().isAutoReloadMapEnabled());
    _hotReloadUpdateOnChangeCheckbox    ->SetValue(Impl().isAlwaysUpdateMapEnabled());
}

}
