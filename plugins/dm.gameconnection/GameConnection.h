#pragma once

#include "icommandsystem.h"
#include "iscenegraph.h"
#include "ui/ieventmanager.h"

#include "MapObserver.h"

#include <sigc++/connection.h>
#include <wx/timer.h>

namespace gameconn
{

class MessageTcp;
class AutomationEngine;

/**
 * stgatilov: This is TheDarkMod-only system for connecting to game process via socket.
 * It allows features like:
 *  - immediate camera synchronization
 *  - updating edited entities in game immediately (aka "hot reload")
 */
class GameConnection :
    public wxEvtHandler, //note: must be inherited first, according to wxWidgets docs
    public RegisterableModule
{
public:
    GameConnection();
    ~GameConnection();

    // Connect to TDM instance if not connected yet.
    // Returns false if failed to connect.
    bool connect();
    // Disconnect from TDM instance if connected.
    // If force = true, then it blocks until pending requests are finished.
    // If force = false, then all pending requests are dropped (no blocking for sure).
    void disconnect(bool force = false);
    // Returns false if connection is not yet established or has been closed recently.
    bool isAlive() const;

    // Starts async procedure including:
    //   * connect to existing game instance or start a new one
    //   * set current mod/mission and map
    //   * optionally dmap it
    //   * make sure game is started afresh
    void restartGame(bool dmap);
    // Returns true iff restartGame sequence is currently in progress.
    bool isGameRestarting() const;

    // Enable/disable continuous sync of camera: update in-game player to DarkRadiant camera.
    void setCameraSyncEnabled(bool enable);
    // Returns true iff continuous camera sync is enabled right now.
    bool isCameraSyncEnabled() const;
    // Trigger one-off sync of game position back to DarkRadiant camera.
    void backSyncCamera();

    // Ask game to reload .map file from disk (right now, once).
    void reloadMap();
    // Enable/disable mode: force game to reload .map from disk every time DarkRadiant saves it.
    void setAutoReloadMapEnabled(bool enable);
    // Returns true iff .map reload mode is enabled.
    bool isAutoReloadMapEnabled() const;

    // Enable/disable listening for all entity changes for "update map" feature.
    // See doUpdateMap for more details.
    void setUpdateMapObserverEnabled(bool on);
    // Returns true iff observer for "update map" is enabled.
    bool isUpdateMapObserverEnabled() const;
    // Send pending changes of map entities to game (right now, once).
    // All changes a) since last successful call of this method,
    // or b) since the observer was enabled; are sent as a diff.
    // The game applies the diff on top of its current map state and hot reloads entities.
    void doUpdateMap();
    // Enable/disable mode: doUpdateMap after every entity change.
    // Note: the update is postponed to next think, so that mass changes go as one diff.
    void setAlwaysUpdateMapEnabled(bool on);
    // Returns true iff the mode for update map after every change is enabled.
    bool isAlwaysUpdateMapEnabled() const;

    // Toggle game pause status: pause if it is live / unpause if it is paused.
    // Note: there is no way to learn if game is paused right now.
    void togglePauseGame();
    // Respawn all entities in the current selection set.
    void respawnSelectedEntities();

    // This signal is emitted when status changes:
    //  * connected/disconnected
    //  * restart game starts/ends
    //  * any mode starts/ends
    // GUI should update itself every time it is triggered.
    sigc::signal<void, int> signal_StatusChanged;

    //RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:

    // Underlying engine for connection to TDM game.
    std::unique_ptr<AutomationEngine> _engine;
    // When connected, this timer calls "think" method periodically.
    std::unique_ptr<wxTimer> _thinkTimer;
    // Flag is used to block new timer events if an old timer event has not finished yet.
    bool _timerInProgress = false;

    // Signal subscription for when map is saved, loaded, unloaded, etc.
    sigc::connection _mapEventListener;

    // True iff _cameraOutData holds new camera position, which should be sent to game soon.
    bool _cameraOutPending = false;
    // Data for camera position (setviewpos format: X Y Z -pitch yaw roll)
    Vector3 _cameraOutData[2];
    // Signal subscription for when camera of DarkRadiant view changes.
    sigc::connection _cameraChangedSignal;

    // Observes over changes to map data (mainly spawnargs of entities).
    MapObserver _mapObserver;
    // True when "setAutoReloadMapEnabled" is enabled.
    bool _autoReloadMap = false;
    // True when "setAlwaysUpdateMapEnabled" is enabled.
    bool _updateMapAlways = false;

    // True when restartGame procedure is executed.
    bool _restartInProgress = false;

    // IEventPtrs corresponding to activatable menu options.
    IEventPtr _event_toggleCameraSync;
    IEventPtr _event_backSyncCamera;

private:

    // Add any required items to the application toolbars.
    void addToolbarItems();

    // Enable/disable timer calling think method regularly.
    void setThinkLoop(bool enable);
    // Callback for _thinkTimer.
    void onTimerEvent(wxTimerEvent& ev);
    // Check how socket is doing, accept responses and send pending async requests.
    // This should be done regularly: in fact, timer calls it often.
    void think();
    // If there are any pending async commands (e.g. camera update), send one now.
    // Returns true iff anything was sent to game.
    bool sendAnyPendingAsync();

    // Given a command to be executed in game console (no EOLs), returns its full request text.
    // The result is ready to be sent over to AutomationEngine, which will prepend seqno automatically.
    static std::string composeConExecRequest(std::string consoleLine);

    // Waits for previous TAG_GENERIC requests to finish, then executes request as TAG_GENERIC (blocking).
    std::string executeGenericRequest(const std::string& request);
    // Set noclip or god or notarget to specific state (blocking).
    // toggleCommand is the command which toggles state.
    // offKeyword is the part of phrase printed to game console when the state becomes disabled.
    void executeSetTogglableFlag(const std::string &toggleCommand, bool enable, const std::string &offKeyword);
    // Learn state of the specified cvar (blocking).
    std::string executeGetCvarValue(const std::string &cvarName, std::string *defaultValue = nullptr);
    // Learn current status: installed mod/map, active gui, etc. (blocking).
    std::map<std::string, std::string> executeQueryStatus();

    // Called from camera modification callback: schedules async "setviewpos" action for future.
    void updateCamera();
    // Send request for camera update, which is pending yet.
    bool sendPendingCameraUpdate();
    // Enable notarget/god/noclip to allow player to fly around without problems.
    void enableGhostMode();

    // Save map using DarkRadiant command if there are any pending modifications.
    void saveMapIfNeeded();
    // Callback called on map saving, loading and unloading.
    void onMapEvent(IMap::MapEvent ev);
};

}
