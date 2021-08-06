#pragma once

#include "icommandsystem.h"
#include "iscenegraph.h"
#include "ieventmanager.h"

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

    /**
     * Restart game and connect to it.
     */
    void restartGame(bool dmap);
    //returns true if restartGame sequence is in progress
    bool isGameRestarting() const;


    //connect to TDM instance if not connected yet
    //return false if failed to connect
    bool connect();
    //disconnect from TDM instance if connected
    //if force = true, then it blocks until pending requests are finished
    //if force = false, then all pending requests are dropped, no blocking for sure
    void disconnect(bool force = false);
    //returns false if connection is not yet established or has been closed for whatever reason
    bool isAlive() const;

    /**
     * \brief
     * Enable dynamic sync of camera to game position
     *
     * \return
     * true on success, false if connection failed.
     */
    bool setCameraSyncEnabled(bool enable);

    bool isCameraSyncEnabled() const;

    /// Trigger one-off sync of game position back to Radiant camera
    void backSyncCamera();

    //pause game if it is live, unpause if it is paused
    void togglePauseGame();
    //respawn all entities in the current selection set
    void respawnSelectedEntities();

    //ask TDM to reload .map file from disk
    void reloadMap();

    /**
     * \brief
     * Instruct TDM to reload .map from disk automatically after every map save
     *
     * \return
     * true on success, false if the game connection failed.
     */
    bool setAutoReloadMapEnabled(bool enable);

    bool isAutoReloadMapEnabled() const;

    /**
     * \brief
     * Enable hot reload of map entity changes.
     *
     * \return
     * true on success, false if the game connection failed.
     */
    void setUpdateMapObserverEnabled(bool on);

    bool isUpdateMapObserverEnabled() const;

    bool setAlwaysUpdateMapEnabled(bool on);

    bool isAlwaysUpdateMapEnabled() const;

    //send map update to TDM right now
    void doUpdateMap();

    //signal is emitted when status changes:
    //  connected/disconnected
    //  UpdateMap mode is enabled/disabled
    //  restart game starts/ends
    sigc::signal<void, int> signal_StatusChanged;

    //RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:

    // Add any required items to the application toolbars
    void addToolbarItems();

    // IEventPtrs corresponding to activatable menu options
    IEventPtr _camSyncToggle;
    IEventPtr _camSyncBackButton;

    //underlying engine for connection to TDM game
    std::unique_ptr<AutomationEngine> _engine;
    //when connected, this timer calls Think periodically
    std::unique_ptr<wxTimer> _thinkTimer;
    bool _timerInProgress = false;

    void onTimerEvent(wxTimerEvent& ev);

    //signal listener for when map is saved, loaded, unloaded, etc.
    sigc::connection _mapEventListener;

    //true <=> cameraOutData holds new camera position, which should be sent to TDM
    bool _cameraOutPending = false;
    //data for camera position (setviewpos format: X Y Z -pitch yaw roll)
    Vector3 _cameraOutData[2];
    //the update subscription used when camera sync is enabled
    sigc::connection _cameraChangedSignal;

    //observes over changes to map data
    MapObserver _mapObserver;
    //set to true when "reload map automatically" is on
    bool _autoReloadMap = false;
    //set to true when "update map" is set to "always"
    bool _updateMapAlways = false;

    //set to true when restartGame procedure is executed
    bool _restartInProgress = false;

    //if there are any pending async commands (camera update), send one now
    //returns true iff anything was sent to game
    bool sendAnyPendingAsync();
    //check how socket is doing, accept responses and send pending async requests 
    //this should be done regularly: in fact, timer calls it often
    void think();
    //enable/disable timer calling think method regularly
    void setThinkLoop(bool enable);

    //waits for previous TAG_GENERIC requests to finish, then executes request as TAG_GENERIC in blocking fashion
    std::string executeGenericRequest(const std::string& request);
    //given a command to be executed in game console (no EOLs), returns its full request text (except for seqno)
    static std::string composeConExecRequest(std::string consoleLine);
    //set noclip/god/notarget to specific state (blocking)
    //toggleCommand is the command which toggles state
    //offKeyword is the part of phrase printed to game console when the state becomes disabled
    void executeSetTogglableFlag(const std::string &toggleCommand, bool enable, const std::string &offKeyword);
    //learn state of the specified cvar (blocking)
    std::string executeGetCvarValue(const std::string &cvarName, std::string *defaultValue = nullptr);
    //learn current status: installed mod/map, active gui, etc. (blocking)
    std::map<std::string, std::string> executeQueryStatus();

    //called from camera modification callback: schedules async "setviewpos" action for future
    void updateCamera();
    //send request for camera update, which is pending yet
    bool sendPendingCameraUpdate();
    //enable notarget/god/noclip to allow player to fly around without problems
    void enableGhostMode();

    //saves map using DR code if there are pending modifications
    void saveMapIfNeeded();
    //signal observer on map saving
    void onMapEvent(IMap::MapEvent ev);
};

}
