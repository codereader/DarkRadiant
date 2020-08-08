#include <zmq.hpp>
#undef min
#undef max
#include "GameConnection.h"
#include "MessageTCP.h"
#include "Camera/GlobalCamera.h"


zmq::context_t g_ZeroMqContext;
GameConnection g_gameConnection;


static std::string SeqnoPreamble(int seq) {
    return fmt::format("seqno {}\n", seq);
}
static std::string MessagePreamble(std::string type) {
    return fmt::format("message \"{}\"\n", type);
}
static std::string ActionPreamble(std::string type) {
    return MessagePreamble("action") + fmt::format("action \"{}\"\n", type);
}
static std::string QueryPreamble(std::string type) {
    return MessagePreamble("query") + fmt::format("query \"{}\"\n", type);
}


void GameConnection::SendRequest(const std::string &request) {
    assert(_seqnoInProgress == 0);
    int seqno = g_gameConnection.NewSeqno();
    std::string fullMessage = SeqnoPreamble(seqno) + request;
    _connection->WriteMessage(fullMessage.data(), fullMessage.size());
    _seqnoInProgress = seqno;
}

bool GameConnection::SendAnyAsync() {
    if (_cameraOutPending) {
        std::string text;
        text = ActionPreamble("conexec");
        text += "content:\n";
        text += fmt::format(
            "setviewpos  {:0.3f} {:0.3f} {:0.3f}  {:0.3f} {:0.3f} {:0.3f}\n",
            _cameraOutData[0].x(), _cameraOutData[0].y(), _cameraOutData[0].z(),
            -_cameraOutData[1].x(), _cameraOutData[1].y(), _cameraOutData[1].z()
        );
        SendRequest(text);
        _cameraOutPending = false;
        return true;
    }
    return false;
}

void GameConnection::Think() {
    _connection->Think();
    if (_seqnoInProgress) {
        //check if full response is here
        if (_connection->ReadMessage(_response)) {
            //validate and remove preamble
            int responseSeqno, lineLen;
            int ret = sscanf(_response.data(), "response %d\n%n", &responseSeqno, &lineLen);
            assert(ret == 1);
            assert(responseSeqno == _seqnoInProgress);
            _response.erase(_response.begin(), _response.begin() + lineLen);
            //mark request as "no longer in progress"
            //note: response can be used in outer function
            _seqnoInProgress = 0;
        }
    }
    else {
        //doing nothing now: send async command if present
        bool sentAsync = SendAnyAsync();
        sentAsync = false;  //unused
    }
    _connection->Think();
}

void GameConnection::WaitAction() {
    while (_seqnoInProgress)
        Think();
}


void GameConnection::Finish() {
    //wait for current request in progress to finish
    WaitAction();
    //send pending async commands and wait for them to finish
    while (SendAnyAsync())
        WaitAction();
}

std::string GameConnection::Execute(const std::string &request) {
    //make sure current request is finished (if any)
    WaitAction();
    assert(_seqnoInProgress == 0);

    //prepend seqno line and send message
    SendRequest(request);

    //wait until response is ready
    WaitAction();

    return std::string(_response.begin(), _response.end());
}

bool GameConnection::Connect() {
    if (_connection && _connection->IsAlive())
        return true;    //already connected

    if (_connection)
        _connection.reset();

    //connection using ZeroMQ socket
    std::unique_ptr<zmq::socket_t> zmqConnection(new zmq::socket_t(g_ZeroMqContext, ZMQ_STREAM));
    zmqConnection->connect(fmt::format("tcp://127.0.0.1:{}", 3879));

    _connection.reset(new MessageTcp());
    _connection->Init(std::move(zmqConnection));
    return _connection->IsAlive();
}


void GameConnection::ReloadMap(const cmd::ArgumentList& args) {
    if (!g_gameConnection.Connect())
        return;
    std::string text;
    text = ActionPreamble("conexec");
    text += "content:\n";
    text += "reloadMap\n";  //that's the console command we would execute
    g_gameConnection.Execute(text);
}

void GameConnection::UpdateCamera() {
    Connect();
    if (auto pCamWnd = GlobalCamera().getActiveCamWnd()) {
        Vector3 orig = pCamWnd->getCameraOrigin(), angles = pCamWnd->getCameraAngles();
        _cameraOutData[0] = orig;
        _cameraOutData[1] = angles;
        //note: the update is not necessarily sent right now
        _cameraOutPending = true;
    }
    Think();
}

class GameConnectionCameraObserver : public CameraObserver {
public:
    GameConnection *_owner = nullptr;
    GameConnectionCameraObserver(GameConnection *owner) : _owner(owner) {}
	virtual void cameraMoved() override {
        _owner->UpdateCamera();
    }
};
void GameConnection::SetCameraObserver(bool enable) {
    if (enable && !_cameraObserver)
        _cameraObserver.reset(new GameConnectionCameraObserver(&g_gameConnection));
    if (!enable && _cameraObserver)
        _cameraObserver.reset();
}
void GameConnection::EnableCameraSync(const cmd::ArgumentList& args) {
    g_gameConnection.SetCameraObserver(true);
    GlobalCamera().addCameraObserver(g_gameConnection.GetCameraObserver());
}
void GameConnection::DisableCameraSync(const cmd::ArgumentList& args) {
    GlobalCamera().removeCameraObserver(g_gameConnection.GetCameraObserver());
    g_gameConnection.SetCameraObserver(false);
}
