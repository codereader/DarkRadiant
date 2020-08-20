#include <zmq.hpp>
#undef min
#undef max
#include "GameConnection.h"
#include "MessageTCP.h"
#include "Camera/GlobalCamera.h"
#include "inode.h"
#include "ientity.h"
#include "map/Map.h"
#include "modulesystem/StaticModule.h"
#include <sigc++/signal.h>


//note: I have no idea where to put it
//I guess if another user of ZeroMQ appears,
//then this context should go to independent common place.
zmq::context_t g_ZeroMqContext;


namespace gameconn {

//this is how often this class "thinks" when idle
static const int THINK_INTERVAL = 123;

static std::string seqnoPreamble(int seq) {
    return fmt::format("seqno {}\n", seq);
}
static std::string messagePreamble(std::string type) {
    return fmt::format("message \"{}\"\n", type);
}
static std::string actionPreamble(std::string type) {
    return messagePreamble("action") + fmt::format("action \"{}\"\n", type);
}
static std::string queryPreamble(std::string type) {
    return messagePreamble("query") + fmt::format("query \"{}\"\n", type);
}


int GameConnection::newSeqno() {
    return ++_seqno;
}

void GameConnection::sendRequest(const std::string &request) {
    assert(_seqnoInProgress == 0);
    int seqno = newSeqno();
    std::string fullMessage = seqnoPreamble(seqno) + request;
    _connection->writeMessage(fullMessage.data(), fullMessage.size());
    _seqnoInProgress = seqno;
}

bool GameConnection::sendAnyPendingAsync() {
    if (_mapObserver.getChanges().size() && _updateMapAlways) {
        //note: this is blocking
        doUpdateMap();
        return true;
    }
    return sendPendingCameraUpdate();
}

void GameConnection::think() {
    _connection->think();
    if (_seqnoInProgress) {
        //check if full response is here
        if (_connection->readMessage(_response)) {
            //validate and remove preamble
            int responseSeqno, lineLen;
            int ret = sscanf(_response.data(), "response %d\n%n", &responseSeqno, &lineLen);
            assert(ret == 1); ret;
            assert(responseSeqno == _seqnoInProgress);
            _response.erase(_response.begin(), _response.begin() + lineLen);
            //mark request as "no longer in progress"
            //note: response can be used in outer function
            _seqnoInProgress = 0;
        }
    }
    else {
        //doing nothing now: send async command if present
        bool sentAsync = sendAnyPendingAsync();
        sentAsync = false;  //unused
    }
    _connection->think();
}

void GameConnection::waitAction() {
    while (_seqnoInProgress)
        think();
}

void GameConnection::finish() {
    //wait for current request in progress to finish
    waitAction();
    //send pending async commands and wait for them to finish
    while (sendAnyPendingAsync())
        waitAction();
}

std::string GameConnection::executeRequest(const std::string &request) {
    //make sure current request is finished (if any)
    waitAction();
    assert(_seqnoInProgress == 0);

    //prepend seqno line and send message
    sendRequest(request);

    //wait until response is ready
    waitAction();

    return std::string(_response.begin(), _response.end());
}

bool GameConnection::connect() {
    if (_connection && _connection->isAlive())
        return true;    //already connected

    if (_connection)
        _connection.reset();
    if (_thinkTimer)
        _thinkTimer.reset();
    if (_mapEventListener)
        _mapEventListener.reset();

    //connection using ZeroMQ socket
    std::unique_ptr<zmq::socket_t> zmqConnection(new zmq::socket_t(g_ZeroMqContext, ZMQ_STREAM));
    zmqConnection->connect(fmt::format("tcp://127.0.0.1:{}", 3879));

    _connection.reset(new MessageTcp());
    _connection->init(std::move(zmqConnection));
    if (!_connection->isAlive())
        return false;

    _thinkTimer.reset(new wxTimer());
    _thinkTimer->Connect(wxEVT_TIMER, wxTimerEventHandler(GameConnection::onTimerEvent), NULL, this);
    _thinkTimer->Start(THINK_INTERVAL);

    _mapEventListener.reset(new sigc::connection(
        GlobalMap().signal_mapEvent().connect(
            sigc::mem_fun(*this, &GameConnection::onMapEvent)
        )
    ));

    return true;
}

void GameConnection::disconnect() {
    setUpdateMapLevel(false, false);
    setCameraSyncEnabled(false);
    if (_connection) {
        finish();
        _connection.reset();
    }
    if (_thinkTimer) {
        _thinkTimer->Stop();
        _thinkTimer.reset();
    }
    if (_mapEventListener)
        _mapEventListener.reset();
}

GameConnection::~GameConnection() {
    disconnect();
};

//-------------------------------------------------------------

const std::string& GameConnection::getName() const {
    static std::string _name = MODULE_GAMECONNECTION;
    return _name;
}
const StringSet& GameConnection::getDependencies() const {
    static StringSet _dependencies;
    if (_dependencies.empty()) {
        _dependencies.insert(MODULE_CAMERA);
        _dependencies.insert(MODULE_COMMANDSYSTEM);
        _dependencies.insert(MODULE_MAP);
        _dependencies.insert(MODULE_SCENEGRAPH);
    }
    return _dependencies;
}
void GameConnection::initialiseModule(const ApplicationContext& ctx) {
}
void GameConnection::shutdownModule() {
    disconnect();
}
module::StaticModule<GameConnection> gameConnectionModule;

//-------------------------------------------------------------

std::string GameConnection::composeConExecRequest(std::string consoleLine) {
    assert(consoleLine.find('\n') == std::string::npos);
    return actionPreamble("conexec") + "content:\n" + consoleLine + "\n";
}

void GameConnection::executeSetTogglableFlag(const std::string &toggleCommand, bool enable, const std::string &offKeyword) {
    if (!connect())
        return;
    std::string text = composeConExecRequest(toggleCommand);
    int attempt;
    for (attempt = 0; attempt < 2; attempt++) {
        std::string response = executeRequest(text);
        bool isEnabled = (response.find(offKeyword) == std::string::npos);
        if (enable == isEnabled)
            break;
        //wrong state: toggle it again
    }
    assert(attempt < 2);    //two toggles not enough?...
}

std::string GameConnection::executeGetCvarValue(const std::string &cvarName, std::string *defaultValue) {
    if (!connect())
        return "";
    std::string text = composeConExecRequest(cvarName);
    std::string response = executeRequest(text);
    //parse response (imagine how easy that would be with regex...)
    while (response.size() && isspace(response.back()))
        response.pop_back();
    std::string expLeft = fmt::format("\"{}\" is:\"", cvarName);
    std::string expMid = "\" default:\"";
    std::string expRight = "\"";
    int posLeft = response.find(expLeft);
    int posMid = response.find(expMid);
    if (posLeft < 0 || posMid < 0) {
        rError() << fmt::format("ExecuteGetCvarValue: can't parse value of {}", cvarName);
        return "";
    }
    int posLeftEnd = posLeft + expLeft.size();
    int posMidEnd = posMid + expMid.size();
    int posRight = response.size() - expRight.size();
    std::string currValue = response.substr(posLeftEnd, posMid - posLeftEnd);
    std::string defValue = response.substr(posMidEnd, posRight - posMidEnd);
    //return results
    if (defaultValue)
        *defaultValue = defValue;
    return currValue;
}

void GameConnection::updateCamera() {
    connect();
    if (auto camWnd = GlobalCamera().getActiveCamWnd()) {
        Vector3 orig = camWnd->getCameraOrigin(), angles = camWnd->getCameraAngles();
        _cameraOutData[0] = orig;
        _cameraOutData[1] = angles;
        //note: the update is not necessarily sent right now
        _cameraOutPending = true;
    }
    think();
}
bool GameConnection::sendPendingCameraUpdate() {
    if (_cameraOutPending) {
        std::string text = composeConExecRequest(fmt::format(
            "setviewpos  {:0.3f} {:0.3f} {:0.3f}  {:0.3f} {:0.3f} {:0.3f}",
            _cameraOutData[0].x(), _cameraOutData[0].y(), _cameraOutData[0].z(),
            -_cameraOutData[1].x(), _cameraOutData[1].y(), _cameraOutData[1].z()
        ));
        sendRequest(text);
        _cameraOutPending = false;
        return true;
    }
    return false;
}
class GameConnection_CameraObserver : public CameraObserver {
    GameConnection &_owner;
public:
    GameConnection_CameraObserver(GameConnection &owner) : _owner(owner) {}
    virtual void cameraMoved() override {
        _owner.updateCamera();
    }
};
void GameConnection::setCameraSyncEnabled(bool enable) {
    if (!enable && _cameraObserver) {
        GlobalCamera().removeCameraObserver(_cameraObserver.get());
        _cameraObserver.reset();
    }
    if (enable) {
        if (!_cameraObserver) {
            _cameraObserver.reset(new GameConnection_CameraObserver(*this));
            GlobalCamera().addCameraObserver(_cameraObserver.get());
        }
        executeSetTogglableFlag("god", true, "OFF");
        executeSetTogglableFlag("noclip", true, "OFF");
        executeSetTogglableFlag("notarget", true, "OFF");
        //sync camera location right now
        updateCamera();
        finish();
    }
}
void GameConnection::backSyncCamera() {
    if (!connect())
        return;
    std::string text = executeRequest(composeConExecRequest("getviewpos"));
    Vector3 orig, angles;
    if (sscanf(text.c_str(), "%lf%lf%lf%lf%lf%lf", &orig.x(), &orig.y(), &orig.z(), &angles.x(), &angles.y(), &angles.z()) == 6) {
        if (auto camWnd = GlobalCamera().getActiveCamWnd()) {
            angles.x() *= -1.0;
            camWnd->setCameraOrigin(orig);
            camWnd->setCameraAngles(angles);
        }
    }
}

void GameConnection::togglePauseGame() {
    if (!connect())
        return;
    std::string value = executeGetCvarValue("g_stopTime");
    std::string oppositeValue = (value == "0" ? "1" : "0");
    std::string text = composeConExecRequest(fmt::format("g_stopTime {}", oppositeValue));
    executeRequest(text);
}

void GameConnection::reloadMap() {
    if (!connect())
        return;
    std::string text = composeConExecRequest("reloadMap");
    executeRequest(text);
}
void GameConnection::onMapEvent(IMap::MapEvent ev) {
    if (ev == IMap::MapSaved && _autoReloadMap) {
        reloadMap();
        _mapObserver.clear();
    }
    if (ev == IMap::MapLoading || ev == IMap::MapUnloading) {
        disconnect();
    }
}
void GameConnection::setAutoReloadMapEnabled(bool enable) {
    _autoReloadMap = enable;
}
void GameConnection::setUpdateMapLevel(bool on, bool always) {
    if (on && !_mapObserver.isEnabled()) {
        //save map to file, and reload from file, to ensure DR and TDM are in sync
        GlobalCommandSystem().executeCommand("SaveMap");
        reloadMap();
    }
    _mapObserver.setEnabled(on);
    _updateMapAlways = always;
}
void GameConnection::doUpdateMap() {
    if (!connect())
        return;
    std::string diff = GlobalMap().saveMapDiff(_mapObserver.getChanges());
    if (diff.empty()) {
        return; //TODO: fail
    }
    std::string response = executeRequest(actionPreamble("reloadmap-diff") + "content:\n" + diff);
    if (response.find("HotReload: SUCCESS") != std::string::npos)
        _mapObserver.clear();
}

}
