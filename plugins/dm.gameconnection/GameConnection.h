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

    //connect to TDM instance if not connected yet
    //return false if failed to connect
    bool connect();
    //disconnect from TDM instance if connected
    //if force = true, then it blocks until pending requests are finished
    //if force = false, then all pending requests are dropped, no blocking for sure
    void disconnect(bool force = false);
    //returns false if connection is not yet established or has been closed for whatever reason
    bool isAlive() const;

    //flush all async commands (e.g. camera update) and wait until everything finishes
    void finish();

    /**
     * \brief
     * Enable dynamic sync of camera to game position
     *
     * \return
     * true on success, false if connection failed.
     */
    bool setCameraSyncEnabled(bool enable);

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

    /**
     * \brief
     * Enable hot reload of map entity changes.
     *
     * \return
     * true on success, false if the game connection failed.
     */
    bool setMapHotReload(bool on);

    //send map update to TDM right now
    void doUpdateMap();

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

    //connection to TDM game (i.e. the socket with custom message framing)
    //it can be "dead" in two ways:
    //  _connection is NULL --- no connection, all modes/observers disabled
    //  *_connection is dead --- just lost connection, must call "disconnect" ASAP to disable modes/observers
    std::unique_ptr<MessageTcp> _connection;
    //when connected, this timer calls Think periodically
    std::unique_ptr<wxTimer> _thinkTimer;
    bool _timerInProgress;

    void onTimerEvent(wxTimerEvent& ev);

    //signal listener for when map is saved, loaded, unloaded, etc.
    sigc::connection _mapEventListener;
    //sequence number of the last sent request (incremented sequentally)
    std::size_t _seqno = 0;

    //nonzero: request with this seqno sent to game, response not recieved yet
    std::size_t _seqnoInProgress = 0;
    //response from current in-progress request will be saved here
    std::vector<char> _response;

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

    // Enable or disable the map observer
    void activateMapObserver(bool on);

    //every request should get unique seqno, otherwise we won't be able to distinguish their responses
    std::size_t generateNewSequenceNumber();
    //prepend seqno to specified request and send it to game
    void sendRequest(const std::string &request);
    //if there are any pending async commands (camera update), send one now
    //returns true iff anything was sent to game
    bool sendAnyPendingAsync();
    //check how socket is doing, accept responses and send pending async requests 
    //this should be done regularly: in fact, timer calls it often
    void think();
    //wait until the currently executed request is finished
    void waitAction();
    //send given request synchronously, i.e. wait until its completition (blocking)
    //returns response content
    std::string executeRequest(const std::string &request);

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
    //saves map using DR code if there are pending modifications
    void saveMapIfNeeded();

    //signal observer on map saving
    void onMapEvent(IMap::MapEvent ev);
};

}
