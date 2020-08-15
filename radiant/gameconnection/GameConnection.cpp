#include <zmq.hpp>
#undef min
#undef max
#include "GameConnection.h"
#include "MessageTCP.h"
#include "Camera/GlobalCamera.h"
#include "inode.h"
#include "ientity.h"
#include "map/Map.h"


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


std::string GameConnection::ComposeConExecRequest(std::string consoleLine) {
    assert(consoleLine.find('\n') == std::string::npos);
    return ActionPreamble("conexec") + "content:\n" + consoleLine + "\n";
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
        std::string text = ComposeConExecRequest(fmt::format(
            "setviewpos  {:0.3f} {:0.3f} {:0.3f}  {:0.3f} {:0.3f} {:0.3f}",
            _cameraOutData[0].x(), _cameraOutData[0].y(), _cameraOutData[0].z(),
            -_cameraOutData[1].x(), _cameraOutData[1].y(), _cameraOutData[1].z()
        ));
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


void GameConnection::ExecuteSetTogglableFlag(const std::string &toggleCommand, bool enable, const std::string &offKeyword) {
    if (!g_gameConnection.Connect())
        return;
    std::string text = ComposeConExecRequest(toggleCommand);
    int attempt;
    for (attempt = 0; attempt < 2; attempt++) {
        std::string response = g_gameConnection.Execute(text);
        bool isEnabled = (response.find(offKeyword) == std::string::npos);
        if (enable == isEnabled)
            break;
        //wrong state: toggle it again
    }
    assert(attempt < 2);    //two toggles not enough?...
}

std::string GameConnection::ExecuteGetCvarValue(const std::string &cvarName, std::string *defaultValue) {
    if (!g_gameConnection.Connect())
        return "";
    std::string text = ComposeConExecRequest(cvarName);
    std::string response = g_gameConnection.Execute(text);
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

void GameConnection::ReloadMap(const cmd::ArgumentList& args) {
    if (!g_gameConnection.Connect())
        return;
    std::string text = ComposeConExecRequest("reloadMap");
    g_gameConnection.Execute(text);
}

void GameConnection::PauseGame(const cmd::ArgumentList& args) {
    if (!g_gameConnection.Connect())
        return;
    std::string value = g_gameConnection.ExecuteGetCvarValue("g_stopTime");
    std::string oppositeValue = (value == "0" ? "1" : "0");
    std::string text = ComposeConExecRequest(fmt::format("g_stopTime {}", oppositeValue));
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
    if (enable) {
        ExecuteSetTogglableFlag("god", true, "OFF");
        ExecuteSetTogglableFlag("noclip", true, "OFF");
        ExecuteSetTogglableFlag("notarget", true, "OFF");
        //sync camera location right now
        UpdateCamera();
        Finish();
    }
}
void GameConnection::EnableCameraSync(const cmd::ArgumentList& args) {
    g_gameConnection.SetCameraObserver(true);
    GlobalCamera().addCameraObserver(g_gameConnection.GetCameraObserver());
}
void GameConnection::DisableCameraSync(const cmd::ArgumentList& args) {
    GlobalCamera().removeCameraObserver(g_gameConnection.GetCameraObserver());
    g_gameConnection.SetCameraObserver(false);
}


static std::vector<IEntityNodePtr> GetEntitiesInNode(const scene::INodePtr &node) {
    struct MyVisitor : scene::NodeVisitor {
        std::set<IEntityNodePtr> v;
        virtual bool pre(const scene::INodePtr& node) override {
            if (IEntityNodePtr ptr = std::dynamic_pointer_cast<IEntityNode, scene::INode>(node)) {
                v.insert(ptr);
                return false;
            }
            return true;
        }
    };
    MyVisitor visitor;
    node->traverse(visitor);
    return std::vector<IEntityNodePtr>(visitor.v.begin(), visitor.v.end());
}
class GameConnectionEntityObserver : public Entity::Observer {
public:
    std::string _entityName;
    bool _enabled = false;
    void Enable() { _enabled = true; }
    virtual void onKeyInsert(const std::string& key, EntityKeyValue& value) override {
        if (key == "name")
            _entityName = value.get();      //happens when installing observer
        if (_enabled)
            g_gameConnection.EntityUpdated(_entityName, 0);
    }
    virtual void onKeyChange(const std::string& key, const std::string& val) override {
        if (_enabled) {
            if (key == "name") {
                //renaming is equivalent to deleting old entity and adding new
                g_gameConnection.EntityUpdated(_entityName, -1);
                g_gameConnection.EntityUpdated(val, 1);
            }
            else {
                g_gameConnection.EntityUpdated(_entityName, 0);
            }
        }
    }
    virtual void onKeyErase(const std::string& key, EntityKeyValue& value) override {
        if (_enabled)
            g_gameConnection.EntityUpdated(_entityName, 0);
    }
};
class GameConnectionSceneObserver : public scene::Graph::Observer {
public:
    virtual void onSceneNodeInsert(const scene::INodePtr& node) override {
        auto entityNodes = GetEntitiesInNode(node);
        for (const IEntityNodePtr &entNode : entityNodes)
            g_gameConnection.EntityUpdated(entNode->name(), 1);
        g_gameConnection.SetEntityObservers(entityNodes, true);
    }
    virtual void onSceneNodeErase(const scene::INodePtr& node) override {
        auto entityNodes = GetEntitiesInNode(node);
        g_gameConnection.SetEntityObservers(entityNodes, false);
        for (const IEntityNodePtr &entNode : entityNodes)
            g_gameConnection.EntityUpdated(entNode->name(), -1);
    }
};
void GameConnection::SetEntityObservers(const std::vector<IEntityNodePtr> &entityNodes, bool enable) {
    if (enable) {
        for (auto entNode : entityNodes) {
            if (_entityObservers.count(entNode.get()))
                continue;   //already tracked
            GameConnectionEntityObserver *observer = new GameConnectionEntityObserver();
            entNode->getEntity().attachObserver(observer);
            _entityObservers[entNode.get()] = observer;
            observer->Enable();
        }
    }
    else {
        for (auto entNode : entityNodes) {
            if (!_entityObservers.count(entNode.get()))
                continue;   //not tracked
            Entity::Observer* observer = _entityObservers[entNode.get()];
            entNode->getEntity().detachObserver(observer);
            delete observer;
            _entityObservers.erase(entNode.get());
        }
    }
}
void GameConnection::EntityUpdated(const std::string &name, int type) {
    int &status = _entityChangesPending[name];
    status += type;
    if (std::abs(status) > 1) {
        //added an already added entity, or removed an already removed one: should not happen
        assert(false);
        status = (status < 0 ? -1 : 1);
    }
}
void GameConnection::SetSceneObserver(bool enable) {
    auto entityNodes = GetEntitiesInNode(GlobalSceneGraph().root());
    if (enable) {
        SetEntityObservers(entityNodes, true);
        if (!_sceneObserver)
            _sceneObserver.reset(new GameConnectionSceneObserver());
    }
    else {
        if (_sceneObserver)
            _sceneObserver.reset();
        SetEntityObservers(entityNodes, false);
        assert(_entityObservers.empty());
    }
}
void GameConnection::EnableMapObserver(const cmd::ArgumentList& args) {
    g_gameConnection.SetSceneObserver(true);
    GlobalSceneGraph().addSceneObserver(g_gameConnection.GetSceneObserver());
}
void GameConnection::DisableMapObserver(const cmd::ArgumentList& args) {
    GlobalSceneGraph().removeSceneObserver(g_gameConnection.GetSceneObserver());
    g_gameConnection.SetSceneObserver(false);
}
void GameConnection::UpdateMap(const cmd::ArgumentList& args) {
    if (!g_gameConnection.Connect())
        return;
    std::string diff = GlobalMap().saveMapDiff(g_gameConnection._entityChangesPending);
    if (diff.empty()) {
        return; //TODO: fail
    }

    std::string response = g_gameConnection.Execute(ActionPreamble("reloadmap-diff") + "content:\n" + diff);
    if (response.find("HotReload: SUCCESS") != std::string::npos)
        g_gameConnection._entityChangesPending.clear();
}
