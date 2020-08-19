#include "igameconnection.h"
#include "icommandsystem.h"
#include "iscenegraph.h"
#include "MapObserver.h"


class CameraObserver;

namespace gameconn {

class MessageTcp;

/**
 * stgatilov: This is TheDarkMod-only system for connecting to game process via socket.
 * It allows features like:
 *  - immediate camera synchronization
 *  - updating edited entities in game immediately (aka "hot reload")
 */
class GameConnection :
	public IGameConnection,
	public wxEvtHandler			//note: everything before this base must have no data members!
{
public:
	~GameConnection();

	//connect to TDM instance if not connected yet
	//return false if failed to connect
	bool connect();
	//disconnect from TDM instance if connected
	//note: blocks until pending requests are processed
	void disconnect();

	//flush all async commands (e.g. camera update) and wait until everything finishes
	void finish();

	//make sure camera observer is present iff enable == true, and attach/detach it to global camera
	void setCameraSyncEnabled(bool enable);
	//copy camera position from the game to DR view
	void backSyncCamera();

	//pause game if it is live, unpause if it is paused
	void togglePauseGame();

	//ask TDM to reload .map file from disk
	void reloadMap();
	//implementation of "update map" level toggling
	void setUpdateMapLevel(bool on, bool always);
	//send map update to TDM right now
	void doUpdateMap();

	//RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
	virtual void shutdownModule() override;

private:
	//connection to TDM game (i.e. the socket with custom message framing)
	std::unique_ptr<MessageTcp> _connection;
	//when connected, this timer calls Think periodically
	std::unique_ptr<wxTimer> _thinkTimer;
	void onTimerEvent(wxTimerEvent& ev) { think(); }
	//sequence number of the last sent request (incremented sequentally)
	int _seqno = 0;

	//nonzero: request with this seqno sent to game, response not recieved yet
	int _seqnoInProgress = 0;
	//response from current in-progress request will be saved here
	std::vector<char> _response;

	//true <=> cameraOutData holds new camera position, which should be sent to TDM
	bool _cameraOutPending = false;
	//data for camera position (setviewpos format: X Y Z -pitch yaw roll)
	Vector3 _cameraOutData[2];
	//the observer put onto global camera when camera sync is enabled
	std::unique_ptr<CameraObserver> _cameraObserver;

	//observes over changes to map data
	MapObserver _mapObserver;
	//set to true when "update map" is set to "always"
	bool _updateMapAlways = false;

	
	//every request should get unique seqno, otherwise we won't be able to distinguish their responses
	int newSeqno();
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

	//given a command to be executed in game console (no EOLs), returns its full response text (except for seqno)
	static std::string composeConExecRequest(std::string consoleLine);
	//set noclip/god/notarget to specific state (blocking)
	//toggleCommand is the command which toggles state
	//offKeyword is the part of phrase printed to game console when the state becomes disabled
	void executeSetTogglableFlag(const std::string &toggleCommand, bool enable, const std::string &offKeyword);
	//learn state of the specified cvar (blocking)
	std::string executeGetCvarValue(const std::string &cvarName, std::string *defaultValue = nullptr);

	//called from camera modification callback: schedules async "setviewpos" action for future
	void updateCamera();
	//send request for camera update, which is pending yet
	bool sendPendingCameraUpdate();

	//friend zone:
	friend class GameConnection_CameraObserver;
};

}

using gameconn::GameConnection;
