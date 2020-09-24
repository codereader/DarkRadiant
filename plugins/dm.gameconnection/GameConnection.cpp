#include "GameConnection.h"
#include "MessageTcp.h"
#include "DiffStatus.h"
#include "DiffDoom3MapWriter.h"
#include "clsocket/ActiveSocket.h"

#include "icamera.h"
#include "inode.h"
#include "ientity.h"
#include "iselection.h"
#include "iuimanager.h"
#include "ieventmanager.h"

#include "scene/MapExporter.h"
#include "scene/Traverse.h"

#include <sigc++/signal.h>
#include <sigc++/connection.h>

namespace gameconn
{

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
#if 0
static std::string queryPreamble(std::string type) {
    return messagePreamble("query") + fmt::format("query \"{}\"\n", type);
}
#endif


int GameConnection::newSeqno() {
    return ++_seqno;
}

void GameConnection::sendRequest(const std::string &request) {
    if (!isAlive())
        return; //connection is down, drop all requests
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
    if (!_connection)
        return; //everything disabled, so just don't do anything
    _connection->think();
    if (_seqnoInProgress) {
        //check if full response is here
        if (_connection->readMessage(_response)) {
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
        sendAnyPendingAsync();
    }
    _connection->think();
    if (!_connection->isAlive()) {
        //just lost connection: disable everything
        disconnect(true);
    }
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

bool GameConnection::isAlive() const {
    return _connection && _connection->isAlive();
}

bool GameConnection::connect() {
    if (isAlive())
        return true;    //already connected

    if (_connection) {
        //connection recently lost: disable everything
        disconnect(true);
        assert(!_connection);
    }

    //connection using clsocket
    std::unique_ptr<CActiveSocket> connection(new CActiveSocket());
    if (!connection->Initialize())
        return false;
    if (!connection->SetNonblocking())
        return false;
    //TODO: make post configurable, as it is in TDM?
    if (!connection->Open("localhost", 3879))
        return false;

    _connection.reset(new MessageTcp());
    _connection->init(std::move(connection));
    if (!_connection->isAlive())
        return false;

    _thinkTimer.reset(new wxTimer());
    _thinkTimer->Connect(wxEVT_TIMER, wxTimerEventHandler(GameConnection::onTimerEvent), NULL, this);
    _thinkTimer->Start(THINK_INTERVAL);

    _mapEventListener.reset(new sigc::connection(
        GlobalMapModule().signal_mapEvent().connect(
            sigc::mem_fun(*this, &GameConnection::onMapEvent)
        )
    ));

    return true;
}

void GameConnection::disconnect(bool force) {
    setAutoReloadMapEnabled(false);
    setUpdateMapLevel(false, false);
    setCameraSyncEnabled(false);
    if (force) {
        //drop everything pending 
        _seqnoInProgress = 0;
        _mapObserver.clear();
        _cameraOutPending = false;
    }
    else {
        //try to finish all pending
        finish();
    }
    if (_connection)
        _connection.reset();
    if (_thinkTimer) {
        _thinkTimer->Stop();
        _thinkTimer.reset();
    }
    if (_mapEventListener)
        _mapEventListener.reset();
}

GameConnection::~GameConnection() {
    disconnect(true);
};

//-------------------------------------------------------------

const std::string& GameConnection::getName() const
{
    static std::string _name("GameConnection");
    return _name;
}

const StringSet& GameConnection::getDependencies() const
{
    static StringSet _dependencies {
        MODULE_CAMERA, MODULE_COMMANDSYSTEM, MODULE_MAP, MODULE_SCENEGRAPH,
        MODULE_SELECTIONSYSTEM, MODULE_EVENTMANAGER, MODULE_UIMANAGER
    };
    return _dependencies;
}

void GameConnection::initialiseModule(const ApplicationContext& ctx)
{
    // Add commands
    GlobalCommandSystem().addCommand("GameConnectionCameraSyncEnable",
        [this](const cmd::ArgumentList&) { setCameraSyncEnabled(true); });
    GlobalCommandSystem().addCommand("GameConnectionCameraSyncDisable",
        [this](const cmd::ArgumentList&) { setCameraSyncEnabled(false); });
    GlobalCommandSystem().addCommand("GameConnectionBackSyncCamera",
        [this](const cmd::ArgumentList&) { backSyncCamera(); });
    GlobalCommandSystem().addCommand("GameConnectionReloadMap",
        [this](const cmd::ArgumentList&) { reloadMap(); });
    GlobalCommandSystem().addCommand("GameConnectionReloadMapAutoEnable",
        [this](const cmd::ArgumentList&) { setAutoReloadMapEnabled(true); });
    GlobalCommandSystem().addCommand("GameConnectionReloadMapAutoDisable",
        [this](const cmd::ArgumentList&) { setAutoReloadMapEnabled(false); });
    GlobalCommandSystem().addCommand("GameConnectionUpdateMapOff",
        [this](const cmd::ArgumentList&) { setUpdateMapLevel(false, false); });
    GlobalCommandSystem().addCommand("GameConnectionUpdateMapOn",
        [this](const cmd::ArgumentList&) { setUpdateMapLevel(true, false); });
    GlobalCommandSystem().addCommand("GameConnectionUpdateMapAlways",
        [this](const cmd::ArgumentList&) { setUpdateMapLevel(true, true); });
    GlobalCommandSystem().addCommand("GameConnectionUpdateMap",
        [this](const cmd::ArgumentList&) { doUpdateMap(); });
    GlobalCommandSystem().addCommand("GameConnectionPauseGame",
        [this](const cmd::ArgumentList&) { togglePauseGame(); });
    GlobalCommandSystem().addCommand("GameConnectionRespawnSelected",
        [this](const cmd::ArgumentList&) { respawnSelectedEntities(); });

    // Add events
    GlobalEventManager().addCommand("GameConnectionCameraSyncEnable", "GameConnectionCameraSyncEnable");
    GlobalEventManager().addCommand("GameConnectionCameraSyncDisable", "GameConnectionCameraSyncDisable");
    GlobalEventManager().addCommand("GameConnectionBackSyncCamera", "GameConnectionBackSyncCamera");
    GlobalEventManager().addCommand("GameConnectionReloadMap", "GameConnectionReloadMap");
    GlobalEventManager().addCommand("GameConnectionReloadMapAutoEnable", "GameConnectionReloadMapAutoEnable");
    GlobalEventManager().addCommand("GameConnectionReloadMapAutoDisable", "GameConnectionReloadMapAutoDisable");
    GlobalEventManager().addCommand("GameConnectionUpdateMapOff", "GameConnectionUpdateMapOff");
    GlobalEventManager().addCommand("GameConnectionUpdateMapOn", "GameConnectionUpdateMapOn");
    GlobalEventManager().addCommand("GameConnectionUpdateMapAlways", "GameConnectionUpdateMapAlways");
    GlobalEventManager().addCommand("GameConnectionUpdateMap", "GameConnectionUpdateMap");
    GlobalEventManager().addCommand("GameConnectionPauseGame", "GameConnectionPauseGame");
    GlobalEventManager().addCommand("GameConnectionRespawnSelected", "GameConnectionRespawnSelected");

    // Add menu items
    IMenuManager& mm = GlobalUIManager().getMenuManager();
    mm.add("main", "connection", ui::menuFolder, "Connection", "", "");
    mm.add("main/connection", "cameraSyncEnable", ui::menuItem,
           "Enable camera synchronization", "", "GameConnectionCameraSyncEnable");
    mm.add("main/connection", "cameraSyncDisable", ui::menuItem,
           "Disable camera synchronization", "", "GameConnectionCameraSyncDisable");
    mm.add("main/connection", "backSyncCamera", ui::menuItem,
           "Sync camera back now", "", "GameConnectionBackSyncCamera");
    mm.add("main/connection", "reloadMap", ui::menuItem,
           "Reload map from .map file", "", "GameConnectionReloadMap");
    mm.add("main/connection", "reloadMapAutoEnable", ui::menuItem,
           "Enable automation .map reload on save", "", "GameConnectionReloadMapAutoEnable");
    mm.add("main/connection", "reloadMapAutoDisable", ui::menuItem,
           "Disable automation .map reload on save", "", "GameConnectionReloadMapAutoDisable");
    mm.add("main/connection", "updateMapOff", ui::menuItem,
           "Disable map update mode", "", "GameConnectionUpdateMapOff");
    mm.add("main/connection", "updateMapOn", ui::menuItem,
           "Enable map update mode", "", "GameConnectionUpdateMapOn");
    mm.add("main/connection", "updateMapAlways", ui::menuItem,
           "Always update map immediately after change", "", "GameConnectionUpdateMapAlways");
    mm.add("main/connection", "updateMap", ui::menuItem,
           "Update map right now", "", "GameConnectionUpdateMap");
    mm.add("main/connection", "pauseGame", ui::menuItem,
           "Pause game", "", "GameConnectionPauseGame");
    mm.add("main/connection", "respawnSelected", ui::menuItem,
           "Respawn selected entities", "", "GameConnectionRespawnSelected");
}

void GameConnection::shutdownModule()
{
    disconnect(true);
}

//-------------------------------------------------------------

std::string GameConnection::composeConExecRequest(std::string consoleLine) {
    //remove trailing spaces/EOLs
    while (!consoleLine.empty() && isspace(consoleLine.back()))
        consoleLine.pop_back();
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
    if (auto camWnd = GlobalCameraView().getActiveCameraView()) {
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

class GameConnection_CameraObserver : public ui::CameraObserver
{
    GameConnection &_owner;
public:
    GameConnection_CameraObserver(GameConnection &owner) : _owner(owner) {}
    virtual void cameraMoved() override {
        _owner.updateCamera();
    }
};

void GameConnection::setCameraSyncEnabled(bool enable) {
    if (!enable && _cameraObserver) {
        GlobalCameraView().removeCameraObserver(_cameraObserver.get());
        _cameraObserver.reset();
    }
    if (enable) {
        if (!connect())
            return;
        if (!_cameraObserver) {
            _cameraObserver.reset(new GameConnection_CameraObserver(*this));
            GlobalCameraView().addCameraObserver(_cameraObserver.get());
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
        if (auto camWnd = GlobalCameraView().getActiveCameraView()) {
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

void GameConnection::respawnSelectedEntities() {
    if (!connect())
        return;
    std::set<std::string> selectedEntityNames;
    GlobalSelectionSystem().foreachSelected([&selectedEntityNames](const scene::INodePtr &node) -> void {
        //Node_isEntity
        if (Entity* entity = Node_getEntity(node)) {
            const std::string &name = entity->getKeyValue("name");
            if (!name.empty()) {
                selectedEntityNames.insert(name);
            }
        }
    });
    std::string command;
    for (const std::string &name : selectedEntityNames)
        command += "respawn " + name + "\n";
    std::string text = composeConExecRequest(command);
    executeRequest(text);
}

void GameConnection::reloadMap() {
    if (!connect())
        return;
    std::string text = composeConExecRequest("reloadMap nocheck");
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

/**
 * stgatilov: Saves only entities with specified names to in-memory map patch.
 * This diff is intended to be consumed by TheDarkMod automation for HotReload purposes.
 * TODO: What about patches and brushes?
 */
std::string saveMapDiff(const DiffEntityStatuses& entityStatuses)
{
    //if (_saveInProgress) return "";     // fail if during proper map save

    scene::IMapRootNodePtr root = GlobalSceneGraph().root();

    std::set<scene::INode*> subsetNodes;
    root->foreachNode([&](const scene::INodePtr &node) -> bool {
        if (entityStatuses.count(node->name()))
            subsetNodes.insert(node.get());
        return true;
    });

    std::ostringstream outStream;
    outStream << "// diff " << entityStatuses.size() << std::endl;

    DiffDoom3MapWriter writer;
    writer.setStatuses(entityStatuses);

    //write removal stubs (no actual spawnargs)
    for (const auto &pNS : entityStatuses) {
        const std::string &name = pNS.first;
        DiffStatus status = pNS.second;
        assert(status.isModified());    //(don't put untouched entities into map)
        if (status.isRemoved())
            writer.writeRemoveEntityStub(pNS.first, outStream);
    }

    //write added/modified entities as usual
    map::MapExporterPtr exporter(new map::MapExporter(writer, root, outStream, 0));
    exporter->exportMap(root, map::traverseSubset(subsetNodes));

    return outStream.str();
}

void GameConnection::doUpdateMap() {
    if (!connect())
        return;
    std::string diff = saveMapDiff(_mapObserver.getChanges());
    if (diff.empty()) {
        return; //TODO: fail
    }
    std::string response = executeRequest(actionPreamble("reloadmap-diff") + "content:\n" + diff);
    if (response.find("HotReload: SUCCESS") != std::string::npos)
        _mapObserver.clear();
}

}

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
    module::performDefaultInitialisation(registry);
    registry.registerModule(std::make_shared<gameconn::GameConnection>());
}
