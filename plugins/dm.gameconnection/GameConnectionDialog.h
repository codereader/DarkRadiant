#pragma once

#include "icommandsystem.h"

#include "wxutil/window/TransientWindow.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include <sigc++/connection.h>

class wxCheckBox;
class wxButton;
class wxActivityIndicator;

namespace gameconn
{

class GameConnection;

/**
 * stgatilov: This is top-level non-modal window
 * which displays the status of game connection system,
 * and allows to control its modes and actions.
 */
class GameConnectionDialog :
	public wxutil::TransientWindow,
	private wxutil::XmlResourceBasedWidget
{
    wxCheckBox* _connectedCheckbox               = nullptr;
    wxButton*   _restartGameButton               = nullptr;
    wxCheckBox* _dmapCheckbox                    = nullptr;
    wxButton*   _cameraLoadFromGameButton        = nullptr;
    wxCheckBox* _cameraSendToGameCheckbox        = nullptr;
    wxButton*   _mapFileReloadNowButton          = nullptr;
    wxCheckBox* _mapFileReloadOnSaveCheckbox     = nullptr;
    wxButton*   _hotReloadUpdateNowButton        = nullptr;
    wxCheckBox* _hotReloadUpdateOnChangeCheckbox = nullptr;
    wxButton*   _respawnSelectedButton           = nullptr;
    wxButton*   _pauseGameButton                 = nullptr;
    wxActivityIndicator* _connectedActivityIndicator = nullptr;

    sigc::connection _updateOnStatusChangeSignal;

public:
    ~GameConnectionDialog();

    // This is the actual home of the static instance
    static GameConnectionDialog& Instance();

    // Toggle the visibility of the dialog instance, constructing it if necessary.
    static void toggleDialog(const cmd::ArgumentList& args);

    // Makes all GUI inactive / active depending on whether connection is alive.
    void updateConnectedStatus();

protected:
    // TransientWindow callbacks
    virtual void _preShow() override;
    virtual void _preHide() override;

private:
    GameConnectionDialog();

    GameConnection& Impl();
};

void showError(const std::string& text);

} // namespace ui
