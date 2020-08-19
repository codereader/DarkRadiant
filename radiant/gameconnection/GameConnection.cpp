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


std::string GameConnection::composeConExecRequest(std::string consoleLine) {
    assert(consoleLine.find('\n') == std::string::npos);
    return actionPreamble("conexec") + "content:\n" + consoleLine + "\n";
}

void GameConnection::sendRequest(const std::string &request) {
    assert(_seqnoInProgress == 0);
    int seqno = newSeqno();
    std::string fullMessage = seqnoPreamble(seqno) + request;
    _connection->writeMessage(fullMessage.data(), fullMessage.size());
    _seqnoInProgress = seqno;
}

bool GameConnection::sendAnyPendingAsync() {
    if (_entityChangesPending.size() && _updateMapAlways) {
        //note: this is blocking
        doUpdateMap();
        return true;
    }
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

    return true;
}

void GameConnection::disconnect() {
    if (_connection) {
        finish();
        _connection.reset();
    }
    if (_thinkTimer) {
        _thinkTimer->Stop();
        _thinkTimer.reset();
    }
}

GameConnection& GameConnection::instance() {
    auto ptr = dynamic_cast<GameConnection*>(GlobalGameConnection());
    assert(ptr);
    return *ptr;
}
GameConnection::~GameConnection() {
    disconnect();
};

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

void GameConnection::reloadMap(const cmd::ArgumentList& args) {
    if (!instance().connect())
        return;
    std::string text = composeConExecRequest("reloadMap");
    instance().executeRequest(text);
}

void GameConnection::pauseGame(const cmd::ArgumentList& args) {
    if (!instance().connect())
        return;
    std::string value = instance().executeGetCvarValue("g_stopTime");
    std::string oppositeValue = (value == "0" ? "1" : "0");
    std::string text = composeConExecRequest(fmt::format("g_stopTime {}", oppositeValue));
    instance().executeRequest(text);
}

void GameConnection::updateCamera() {
    connect();
    if (auto pCamWnd = GlobalCamera().getActiveCamWnd()) {
        Vector3 orig = pCamWnd->getCameraOrigin(), angles = pCamWnd->getCameraAngles();
        _cameraOutData[0] = orig;
        _cameraOutData[1] = angles;
        //note: the update is not necessarily sent right now
        _cameraOutPending = true;
    }
    think();
}


class GameConnectionCameraObserver : public CameraObserver {
public:
    GameConnection *_owner = nullptr;
    GameConnectionCameraObserver(GameConnection *owner) : _owner(owner) {}
	virtual void cameraMoved() override {
        _owner->updateCamera();
    }
};
void GameConnection::setCameraObserver(bool enable) {
    if (!enable && _cameraObserver) {
        GlobalCamera().removeCameraObserver(_cameraObserver.get());
        _cameraObserver.reset();
    }
    if (enable) {
        if (!_cameraObserver) {
            _cameraObserver.reset(new GameConnectionCameraObserver(this));
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
void GameConnection::cameraSyncEnable(const cmd::ArgumentList& args) {
    instance().setCameraObserver(true);
}
void GameConnection::cameraSyncDisable(const cmd::ArgumentList& args) {
    instance().setCameraObserver(false);
}


static std::vector<IEntityNodePtr> getEntitiesInNode(const scene::INodePtr &node) {
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
    void enable() { _enabled = true; }
    virtual void onKeyInsert(const std::string& key, EntityKeyValue& value) override {
        if (key == "name")
            _entityName = value.get();      //happens when installing observer
        if (_enabled)
            GameConnection::instance().entityUpdated(_entityName, 0);
    }
    virtual void onKeyChange(const std::string& key, const std::string& val) override {
        if (_enabled) {
            if (key == "name") {
                //renaming is equivalent to deleting old entity and adding new
                GameConnection::instance().entityUpdated(_entityName, -1);
                GameConnection::instance().entityUpdated(val, 1);
            }
            else {
                GameConnection::instance().entityUpdated(_entityName, 0);
            }
        }
    }
    virtual void onKeyErase(const std::string& key, EntityKeyValue& value) override {
        if (_enabled)
            GameConnection::instance().entityUpdated(_entityName, 0);
    }
};
class GameConnectionSceneObserver : public scene::Graph::Observer {
public:
    virtual void onSceneNodeInsert(const scene::INodePtr& node) override {
        auto entityNodes = getEntitiesInNode(node);
        for (const IEntityNodePtr &entNode : entityNodes)
            GameConnection::instance().entityUpdated(entNode->name(), 1);
        GameConnection::instance().setEntityObservers(entityNodes, true);
    }
    virtual void onSceneNodeErase(const scene::INodePtr& node) override {
        auto entityNodes = getEntitiesInNode(node);
        GameConnection::instance().setEntityObservers(entityNodes, false);
        for (const IEntityNodePtr &entNode : entityNodes)
            GameConnection::instance().entityUpdated(entNode->name(), -1);
    }
};
void GameConnection::setEntityObservers(const std::vector<IEntityNodePtr> &entityNodes, bool enable) {
    if (enable) {
        for (auto entNode : entityNodes) {
            if (_entityObservers.count(entNode.get()))
                continue;   //already tracked
            GameConnectionEntityObserver *observer = new GameConnectionEntityObserver();
            entNode->getEntity().attachObserver(observer);
            _entityObservers[entNode.get()] = observer;
            observer->enable();
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
void GameConnection::entityUpdated(const std::string &name, int type) {
    int &status = _entityChangesPending[name];
    status += type;
    if (std::abs(status) > 1) {
        //added an already added entity, or removed an already removed one: should not happen
        assert(false);
        status = (status < 0 ? -1 : 1);
    }
}
void GameConnection::setSceneObserver(bool enable) {
    auto entityNodes = getEntitiesInNode(GlobalSceneGraph().root());
    if (enable) {
        setEntityObservers(entityNodes, true);
        if (!_sceneObserver)
            _sceneObserver.reset(new GameConnectionSceneObserver());
        GlobalSceneGraph().addSceneObserver(_sceneObserver.get());
    }
    else {
        if (_sceneObserver) {
            GlobalSceneGraph().removeSceneObserver(_sceneObserver.get());
            _sceneObserver.reset();
        }
        setEntityObservers(entityNodes, false);
        assert(_entityObservers.empty());
        _entityChangesPending.clear();
    }
}
void GameConnection::setUpdateMapLevel(bool on, bool always) {
    if (on && !_sceneObserver) {
        //save map to file, and reload from file, to ensure DR and TDM are in sync
        GlobalCommandSystem().executeCommand("SaveMap");
        reloadMap(cmd::ArgumentList());
    }
    setSceneObserver(on);
    _updateMapAlways = always;
}
void GameConnection::doUpdateMap() {
    std::string diff = GlobalMap().saveMapDiff(_entityChangesPending);
    if (diff.empty()) {
        return; //TODO: fail
    }
    std::string response = executeRequest(actionPreamble("reloadmap-diff") + "content:\n" + diff);
    if (response.find("HotReload: SUCCESS") != std::string::npos)
        _entityChangesPending.clear();
}
void GameConnection::updateMapOff(const cmd::ArgumentList& args) {
    instance().setUpdateMapLevel(false, false);
}
void GameConnection::updateMapOn(const cmd::ArgumentList& args) {
    instance().setUpdateMapLevel(true, false);
}
void GameConnection::updateMapAlways(const cmd::ArgumentList& args) {
    instance().setUpdateMapLevel(true, true);
}
void GameConnection::updateMap(const cmd::ArgumentList& args) {
    if (!instance().connect())
        return;
    instance().doUpdateMap();
}

}
