#include "icommandsystem.h"

class MessageTcp;
class CameraObserver;

class GameConnection {
public:
	//connect to TDM instance if not connected yet
	//return false if failed to connect
	bool Connect();

	//send given request synchronously, i.e. wait until its completition
	//returns response content
	std::string Execute(const std::string &request);

	//flush all async commands (e.g. camera update) and wait until everything finishes
	void Finish();

	//called from camera modification callback: schedules async "setviewpos" action
	void UpdateCamera();


	static void ReloadMap(const cmd::ArgumentList& args);
	static void EnableCameraSync(const cmd::ArgumentList& args);
	static void DisableCameraSync(const cmd::ArgumentList& args);

private:
	//connection to TDM game (i.e. the socket with custom message framing)
	std::unique_ptr<MessageTcp> _connection;
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


	//every request should get unique seqno, otherwise we won't be able to distinguish their responses
	int NewSeqno() { return ++_seqno; }

	//given a command to be executed in game console (no EOLs), returns its full response text (except for seqno)
	static std::string ComposeConExecRequest(std::string consoleLine);

	//prepend seqno to specified request and send it to game
	void SendRequest(const std::string &request);

	//if there are any pending async commands (camera update), send one now
	//returns true iff anything was sent to game
	bool SendAnyAsync();

	//check how socket is doing, accept responses and send pending async requests 
	void Think();

	//wait until the currently executed request is finished
	void WaitAction();

	//set noclip/god/notarget to specific state (blocking)
	//toggleCommand is the command which toggles state
	//offKeyword is the part of phrase printed to game console when the state becomes disabled
	void ExecuteSetTogglableFlag(const std::string &toggleCommand, bool enable, const std::string &offKeyword);

	//learn state of the specified cvar (blocking)
	std::string ExecuteGetCvarValue(const std::string &cvarName, std::string *defaultValue = nullptr);

	CameraObserver *GetCameraObserver() const { return _cameraObserver.get(); }
	//make sure camera observer is present iff enable == true
	void SetCameraObserver(bool enable);

};
extern GameConnection g_gameConnection;
