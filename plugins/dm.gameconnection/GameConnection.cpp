#include "GameConnection.h"
#include "MessageTcp.h"
#include "DiffStatus.h"
#include "DiffDoom3MapWriter.h"
#include "clsocket/ActiveSocket.h"
#include "GameConnectionDialog.h"

#include "i18n.h"
#include "icameraview.h"
#include "inode.h"
#include "imap.h"
#include "ientity.h"
#include "iselection.h"
#include "imenumanager.h"
#include "ieventmanager.h"
#include "imainframe.h"

#include "scene/Traverse.h"
#include "wxutil/Bitmap.h"
#include "util/ScopedBoolLock.h"
#include "registry/registry.h"

#include <sigc++/signal.h>
#include <sigc++/connection.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/process.h>

namespace gameconn
{

namespace
{
    //this is how often this class "thinks" when idle
    const int THINK_INTERVAL = 123;
    const char* const DEFAULT_HOST = "localhost";
    const int DEFAULT_PORT = 3879;

    inline std::string seqnoPreamble(std::size_t seq) {
        return fmt::format("seqno {0}\n", seq);
    }

    inline std::string messagePreamble(const std::string& type) {
        return fmt::format("message \"{}\"\n", type);
    }

    inline std::string actionPreamble(const std::string& type) {
        return messagePreamble("action") + fmt::format("action \"{0}\"\n", type);
    }

    inline std::string queryPreamble(std::string type) {
        return messagePreamble("query") + fmt::format("query \"{}\"\n", type);
    }
}

GameConnection::GameConnection()
{}

std::size_t GameConnection::generateNewSequenceNumber() {
    return ++_seqno;
}

void GameConnection::sendRequest(const std::string &request) {
    if (!isAlive())
        return; //connection is down, drop all requests
    assert(_seqnoInProgress == 0);
    auto seqno = generateNewSequenceNumber();
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
            assert(static_cast<std::size_t>(responseSeqno) == _seqnoInProgress);
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

void GameConnection::onTimerEvent(wxTimerEvent& ev)
{ 
    if (_timerInProgress) return; // avoid double-entering

    util::ScopedBoolLock guard(_timerInProgress);

    think();
}

void GameConnection::waitAction() {
    while (_seqnoInProgress)
        think();
}

void GameConnection::finish()
{
    do
    {
        //wait for current request in progress to finish
        waitAction();
        //send pending async commands and wait for them to finish
    } while (sendAnyPendingAsync());
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

namespace
{
}

bool GameConnection::connect() {
    if (isAlive())
        return true;    //already connected

    if (_connection) {
        //connection recently lost: disable everything
        disconnect(true);
        assert(!_connection);
    }

    // Make connection using clsocket
    // TODO: make port configurable, as it is in TDM?
    std::unique_ptr<CActiveSocket> connection(new CActiveSocket());
    if (!connection->Initialize()
        || !connection->SetNonblocking()
        || !connection->Open(DEFAULT_HOST, DEFAULT_PORT))
    {
        return false;
    }

    _connection.reset(new MessageTcp());
    _connection->init(std::move(connection));
    if (!_connection->isAlive())
        return false;

    _thinkTimer.reset(new wxTimer());
    _thinkTimer->Bind(wxEVT_TIMER, &GameConnection::onTimerEvent, this);
    _thinkTimer->Start(THINK_INTERVAL);

    _mapEventListener = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(*this, &GameConnection::onMapEvent)
    );

    signal_StatusChanged.emit(0);

    return true;
}

void GameConnection::disconnect(bool force)
{
    _autoReloadMap = false;
    setUpdateMapAlways(false);
    setUpdateMapObserverEnabled(false);
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
    _mapEventListener.disconnect();

    signal_StatusChanged.emit(0);
}

GameConnection::~GameConnection() {
    disconnect(true);
};


void GameConnection::restartGame(bool dmap) {
    //BIG TODO!!
    static const char *TODO_TDM_DIR = R"(G:\TheDarkMod\darkmod)";
    static const char *TODO_MISSION = "bakery";
    static const char *TODO_MAP = "bakery";

    std::string savedViewPos;
    if (isAlive()) {
        //save current position
        savedViewPos = executeRequest(composeConExecRequest("getviewpos"));
    }

    //save .map file
    saveMapIfNeeded();

    //try to attach to TDM with automation enabled
    bool attached = connect();

    if (!attached) {
        //run new TDM process
#ifdef _WIN32
        static const char *TDM_NAME = "TheDarkModx64.exe";
#else
        static const char *TDM_NAME = "thedarkmod.x64";
#endif
        wxString cmdline = wxString::Format("%s +set com_automation 1", TDM_NAME);
        wxExecuteEnv env;
        env.cwd = TODO_TDM_DIR;
        long res = wxExecute(cmdline, wxEXEC_ASYNC, nullptr, &env);
        if (res <= 0) {
            showError("Failed to run TheDarkMod executable.");
            return;
        }

        //attach to the new process
        static const int TDM_LAUNCH_TIMEOUT = 5000;  //in milliseconds
        wxLongLong timestampStart = wxGetUTCTimeMillis();
        do {
            wxMilliSleep(500);
            if (wxGetUTCTimeMillis() - timestampStart > TDM_LAUNCH_TIMEOUT) {
                showError("Timeout when connecting to just started TheDarkMod process.\nMake sure the game is in main menu, has com_automation enabled, and firewall does not block it.");
                return;
            }
        } while (!connect());
    }

    //check the current status
    std::map<std::string, std::string> statusProps = executeQueryStatus();
    if (statusProps["currentfm"] != TODO_MISSION) {
        //change mission/mod and restart TDM engine
        std::string request = actionPreamble("installfm") + "content:\n" + TODO_MISSION + "\n";
        std::string response = executeRequest(request);
        if (response != "done") {
            showError("Failed to change installed mission in TheDarkMod.\nMake sure ?DR mission? is configured properly and game version is 2.09 or above.");
            return;
        }
    }
    statusProps = executeQueryStatus();
    if (statusProps["currentfm"] != TODO_MISSION) {
        showError(fmt::format("Installed mission is {} despite trying to change it.", statusProps["currentfm"]));
        return;
    }

    if (dmap) {
        //run dmap command
        std::string request = composeConExecRequest("dmap " + std::string(TODO_MAP));
        std::string response = executeRequest(request);
        if (response.find("ERROR:") != std::string::npos) {
            showError("Dmap printed error.\nPlease look at TheDarkMod console.");
            return;
        }
    }

    {
        //start map
        std::string request = composeConExecRequest("map " + std::string(TODO_MAP));
        std::string response = executeRequest(request);
    }
    statusProps = executeQueryStatus();
    if (statusProps["currentfm"] != TODO_MISSION) {
        showError(fmt::format("Installed mission is still {}.", statusProps["currentfm"]));
        return;
    }
    if (statusProps["mapname"] != TODO_MISSION) {
        showError(fmt::format("Active map is %s despite trying to start the map.", statusProps["mapname"]));
        return;
    }
    if (statusProps["guiactive"] != TODO_MISSION) {
        showError(fmt::format("GUI %s is active while we expect the game to start", statusProps["guiactive"]));
        return;
    }

    //confirm player is ready
    std::string waitUntilReady = executeGetCvarValue("tdm_player_wait_until_ready");
    if (waitUntilReady != "0") {
        //button0 is "attack" button
        //numbers in parens mean: hold for 100 milliseconds
        std::string request = actionPreamble("gamectrl") + "content:\n" + "timemode astro\n" + "button0 (1 1 0 0 0 100)\n";
        std::string response = executeRequest(request);
    }

    if (!savedViewPos.empty()) {
        //restore camera position
        std::string request = composeConExecRequest(fmt::format("setviewpos {}", savedViewPos));
        std::string response = executeRequest(request);
    }
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
    std::string expLeft = fmt::format("\"{0}\" is:\"", cvarName);
    std::string expMid = "\" default:\"";
    std::string expRight = "\"";
    int posLeft = response.find(expLeft);
    int posMid = response.find(expMid);
    if (posLeft < 0 || posMid < 0) {
        rError() << fmt::format("ExecuteGetCvarValue: can't parse value of {0}", cvarName);
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

std::map<std::string, std::string> GameConnection::executeQueryStatus() {
    std::string request = queryPreamble("status") + "content:\n";
    std::string response = executeRequest(request);

    std::map<std::string, std::string> statusProps;
    int pos = 0;
    while (1) {
        int eolPos = response.find('\n', pos);
        if (eolPos < 0)
            break;
        int spacePos = response.find(' ', pos);
        if (spacePos >= eolPos) {
            rError() << fmt::format("ExecuteQueryStatus: can't parse response");
            return {};
        }

        std::string key = response.substr(pos, spacePos - pos);
        std::string value = response.substr((spacePos + 1), eolPos - (spacePos + 1));
        pos = eolPos + 1;
        statusProps[key] = value;
    }

    return statusProps;
}

void GameConnection::updateCamera() {
    connect();
    try
    {
        auto& camera = GlobalCameraManager().getActiveView();

        auto& orig = camera.getCameraOrigin();
        auto& angles = camera.getCameraAngles();

        _cameraOutData[0] = orig;
        _cameraOutData[1] = angles;
        //note: the update is not necessarily sent right now
        _cameraOutPending = true;
    }
    catch (const std::runtime_error&)
    {
        // no camera
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

bool GameConnection::setCameraSyncEnabled(bool enable)
{
    if (!enable) {
        _cameraChangedSignal.disconnect();
    }
    if (enable) {
        if (!connect())
            return false;
        _cameraChangedSignal.disconnect();
        _cameraChangedSignal = GlobalCameraManager().signal_cameraChanged().connect(
            sigc::mem_fun(this, &GameConnection::updateCamera)
        );
        executeSetTogglableFlag("god", true, "OFF");
        executeSetTogglableFlag("noclip", true, "OFF");
        executeSetTogglableFlag("notarget", true, "OFF");
        //sync camera location right now
        updateCamera();
        finish();
    }
    return true;
}

void GameConnection::backSyncCamera() {
    if (!connect())
        return;
    std::string text = executeRequest(composeConExecRequest("getviewpos"));
    Vector3 orig, angles;
    if (sscanf(text.c_str(), "%lf%lf%lf%lf%lf%lf", &orig.x(), &orig.y(), &orig.z(), &angles.x(), &angles.y(), &angles.z()) == 6) {
        try
        {
            auto& camera = GlobalCameraManager().getActiveView();
            
            angles.x() *= -1.0;
            camera.setOriginAndAngles(orig, angles);
        }
        catch (const std::runtime_error&)
        {
            // no camera view
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
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr &node) {
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

    setUpdateMapObserverEnabled(!GlobalMapModule().isModified());
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

bool GameConnection::setAutoReloadMapEnabled(bool enable)
{
    if (enable && !connect())
        return false;

    _autoReloadMap = enable;
    return true;
}

void GameConnection::saveMapIfNeeded()
{
    if (GlobalMapModule().isModified())
        GlobalCommandSystem().executeCommand("SaveMap");
}

bool GameConnection::isUpdateMapObserverEnabled() const
{
    return _mapObserver.isEnabled();
}

void GameConnection::setUpdateMapObserverEnabled(bool on)
{
    _mapObserver.setEnabled(on);

    signal_StatusChanged.emit(0);
}

bool GameConnection::setUpdateMapAlways(bool on)
{
    if (on && !connect())
        return false;

    setUpdateMapObserverEnabled(on);
    _updateMapAlways = on;
    return true;
}

/**
 * stgatilov: Saves only entities with specified names to in-memory map patch.
 * This diff is intended to be consumed by TheDarkMod automation for HotReload purposes.
 * TODO: What about patches and brushes?
 */
std::string saveMapDiff(const DiffEntityStatuses& entityStatuses)
{
    auto root = GlobalSceneGraph().root();

    std::set<scene::INode*> subsetNodes;
    root->foreachNode([&](const scene::INodePtr& node) {
        if (entityStatuses.count(node->name()))
            subsetNodes.insert(node.get());
        return true;
    });

    std::ostringstream outStream;
    outStream << "// diff " << entityStatuses.size() << std::endl;

    DiffDoom3MapWriter writer(entityStatuses);

    //write removal stubs (no actual spawnargs)
    for (const auto& pNS : entityStatuses) {
        const auto& name = pNS.first;
        const auto& status = pNS.second;
        assert(status.isModified());    //(don't put untouched entities into map)
        if (status.isRemoved())
            writer.writeRemoveEntityStub(name, outStream);
    }

    //write added/modified entities as usual
    {
        registry::ScopedKeyChanger progressDisabler(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG, true);

        // Get a scoped exporter class
        auto exporter = GlobalMapModule().createMapExporter(writer, root, outStream);
        exporter->exportMap(root, scene::traverseSubset(subsetNodes));

        // end the life of the exporter instance here to finish the scene
    }

    return outStream.str();
}

void GameConnection::doUpdateMap()
{
    if (!connect())
        return;

    // Get map diff
    std::string diff = saveMapDiff(_mapObserver.getChanges());
    if (diff.empty()) {
        return; //TODO: fail
    }
    std::string response = executeRequest(actionPreamble("reloadmap-diff") + "content:\n" + diff);
    if (response.find("HotReload: SUCCESS") != std::string::npos)
        _mapObserver.clear();
}

//-------------------------------------------------------------

const std::string& GameConnection::getName() const
{
    static std::string _name("GameConnection");
    return _name;
}

const StringSet& GameConnection::getDependencies() const
{
    static StringSet _dependencies {
        MODULE_CAMERA_MANAGER, MODULE_COMMANDSYSTEM, MODULE_MAP,
        MODULE_SCENEGRAPH, MODULE_SELECTIONSYSTEM, MODULE_EVENTMANAGER,
        MODULE_MENUMANAGER, MODULE_MAINFRAME
    };
    return _dependencies;
}

void GameConnection::initialiseModule(const IApplicationContext& ctx)
{
    // Construct toggles
    _camSyncToggle = GlobalEventManager().addAdvancedToggle(
        "GameConnectionToggleCameraSync",
        [this](bool v) { return setCameraSyncEnabled(v); }
    );
    GlobalEventManager().addAdvancedToggle(
        "GameConnectionToggleAutoMapReload",
        [this](bool v) { return setAutoReloadMapEnabled(v); }
    );
    GlobalEventManager().addAdvancedToggle(
        "GameConnectionToggleHotReload",
        [this](bool v) { return setUpdateMapAlways(v); }
    );

    // Add one-shot commands and associated toolbar buttons
    GlobalCommandSystem().addCommand("GameConnectionBackSyncCamera",
        [this](const cmd::ArgumentList&) { backSyncCamera(); });
    _camSyncBackButton = GlobalEventManager().addCommand(
        "GameConnectionBackSyncCamera", "GameConnectionBackSyncCamera", false
    );
    GlobalCommandSystem().addCommand("GameConnectionReloadMap",
        [this](const cmd::ArgumentList&) { reloadMap(); });
    GlobalCommandSystem().addCommand("GameConnectionUpdateMap",
        [this](const cmd::ArgumentList&) { doUpdateMap(); });
    GlobalCommandSystem().addCommand("GameConnectionPauseGame",
        [this](const cmd::ArgumentList&) { togglePauseGame(); });
    GlobalCommandSystem().addCommand("GameConnectionRespawnSelected",
        [this](const cmd::ArgumentList&) { respawnSelectedEntities(); });

    // Add menu items
    ui::menu::IMenuManager& mm = GlobalMenuManager();
    mm.insert("main/help", "connection", ui::menu::ItemType::Folder, _("Connection"), "", "");

    mm.add("main/connection", "cameraSyncEnable", ui::menu::ItemType::Item,
           _("Game position follows DarkRadiant camera"), "", "GameConnectionToggleCameraSync");
    mm.add("main/connection", "backSyncCamera", ui::menu::ItemType::Item,
           _("Move camera to current game position"), "", "GameConnectionBackSyncCamera");
    mm.add("main/connection", "postCameraSep", ui::menu::ItemType::Separator);

    mm.add("main/connection", "reloadMapAutoEnable", ui::menu::ItemType::Item,
           _("Game reloads .map file on save"), "", "GameConnectionToggleAutoMapReload");
    mm.add("main/connection", "reloadMap", ui::menu::ItemType::Item,
           _("Tell game to reload .map file now"), "", "GameConnectionReloadMap");
    mm.add("main/connection", "postMapFileSep", ui::menu::ItemType::Separator);

    mm.add("main/connection", "mapHotReload", ui::menu::ItemType::Item,
           _("Update entities on every change"), "", "GameConnectionToggleHotReload");
    mm.add("main/connection", "updateMap", ui::menu::ItemType::Item,
           _("Update entities now"), "", "GameConnectionUpdateMap");
    mm.add("main/connection", "postHotReloadSep", ui::menu::ItemType::Separator);

    mm.add("main/connection", "pauseGame", ui::menu::ItemType::Item,
           _("Pause game"), "", "GameConnectionPauseGame");
    mm.add("main/connection", "respawnSelected", ui::menu::ItemType::Item,
           _("Respawn selected entities"), "", "GameConnectionRespawnSelected");

    // Add menu button which shows up the dialog
    GlobalCommandSystem().addCommand("GameConnectionDialogToggle", gameconn::GameConnectionDialog::toggleDialog);
    // Add the menu item
    GlobalMenuManager().add(
        "main/connection", 	// menu location path
        "GameConnectionDialog", // name
        ui::menu::ItemType::Item,	// type
        _("Game Connection..."),	// caption
        "stimresponse.png",	// icon
        "GameConnectionDialogToggle" // event name
    );

    // Toolbar button(s)
    GlobalMainFrame().signal_MainFrameConstructed().connect(
        sigc::mem_fun(this, &GameConnection::addToolbarItems)
    );
}

void GameConnection::shutdownModule()
{
    disconnect(true);
}

void GameConnection::addToolbarItems()
{
    wxToolBar* camTB = GlobalMainFrame().getToolbar(IMainFrame::Toolbar::CAMERA);
    if (camTB)
    {
        // Separate GameConnection tools from regular camera tools
        camTB->AddSeparator();

        // Add toggles for the camera sync functions
        auto camSyncT = camTB->AddTool(
            wxID_ANY, "L", wxutil::GetLocalBitmap("CameraSync.png"),
            _("Enable game camera sync with DarkRadiant camera"),
            wxITEM_CHECK
        );
        _camSyncToggle->connectToolItem(camSyncT);
        auto camSyncBackT = camTB->AddTool(
            wxID_ANY, "B", wxutil::GetLocalBitmap("CameraSyncBack.png"),
            _("Move camera to current game position")
        );
        _camSyncBackButton->connectToolItem(camSyncBackT);

        camTB->Realize();
    }
}

}

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
    module::performDefaultInitialisation(registry);
    registry.registerModule(std::make_shared<gameconn::GameConnection>());
}
