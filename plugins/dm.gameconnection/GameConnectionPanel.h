#pragma once

#include "wxutil/XmlResourceBasedWidget.h"

#include <sigc++/connection.h>

#include "wxutil/DockablePanel.h"

class wxCheckBox;
class wxButton;

//wxActivityIndicator was added in 3.1.0
//so we have to use something else
#define HAVE_ACTIVITYINDICATOR wxUSE_ACTIVITYINDICATOR

#if HAVE_ACTIVITYINDICATOR
    class wxActivityIndicator;
    typedef wxActivityIndicator ActivityIndicatorOrImage;
#else
    class wxStaticBitmap;
    typedef wxStaticBitmap ActivityIndicatorOrImage;
#endif

namespace gameconn { class GameConnection; }

namespace ui
{

/**
 * stgatilov: A panel displaying the status of game connection system,
 * allowing to control its modes and actions.
 */
class GameConnectionPanel :
	public wxutil::DockablePanel,
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
    ActivityIndicatorOrImage* _connectedActivityIndicator = nullptr;

    sigc::connection _updateOnStatusChangeSignal;

public:
    GameConnectionPanel(wxWindow* parent);
    ~GameConnectionPanel() override;

    // Makes all GUI inactive / active depending on whether connection is alive.
    void updateConnectedStatus();

protected:
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();

    gameconn::GameConnection& Impl();
};

} // namespace
