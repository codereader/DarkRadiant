#include "GameConnection.h"
#include "DiffStatus.h"
#include "DiffDoom3MapWriter.h"
#include "AutomationEngine.h"
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

    //all ordinary requests, executed synchronously
    const int TAG_GENERIC = 5;
    //camera DR->TDM sync is continuous, executed asynchronously
    const int TAG_CAMERA = 6;
    //multistep procedure for TDM game start/restart
    const int TAG_RESTART = 7;

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
{
    _engine.reset(new AutomationEngine());
}

std::string GameConnection::executeGenericRequest(const std::string& request)
{
    _engine->waitForTags(TAG_GENERIC);
    return _engine->executeRequestBlocking(TAG_GENERIC, request);
}

bool GameConnection::sendAnyPendingAsync()
{
    if (_mapObserver.getChanges().size() && _updateMapAlways) {
        //note: this is blocking
        doUpdateMap();
        return true;
    }
    return sendPendingCameraUpdate();
}

void GameConnection::think()
{
    if (_engine->hasLostConnection()) {
        //lost connection recently: disable everything
        disconnect(true);
    }

    _engine->think();

    if (!_engine->areTagsInProgress()) {
        //doing nothing now: send async command if present
        sendAnyPendingAsync();
        //think now, don't delay to next frame
        _engine->think();
    }
}

void GameConnection::onTimerEvent(wxTimerEvent& ev)
{ 
    if (_timerInProgress) return; // avoid double-entering

    util::ScopedBoolLock guard(_timerInProgress);

    think();
}

bool GameConnection::isAlive() const
{
    return _engine->isAlive();
}

bool GameConnection::connect()
{
    if (_engine->isAlive())
        return true;        //good already: don't do anything

    if (_engine->hasLostConnection())
        disconnect(true);   //lost connection recently: disable everything

    if (!_engine->connect())
        return false;       //failed to connect

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

    _engine->disconnect(force);
    assert(!_engine->isAlive() && !_engine->hasLostConnection());

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
/*    auto implementation = [this, dmap](int step) -> int {
        //BIG TODO!!
        static const char *TODO_TDM_DIR = R"(G:\TheDarkMod\darkmod)";
        static const char *TODO_MISSION = "bakery_job";
        static const char *TODO_MAP = "bakery.map";

        static bool changingFm;
        static std::string savedViewPos;

        if (step == 0) {

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
                wxExecuteEnv env;
                env.cwd = TODO_TDM_DIR;
                wxString cmdline = wxString::Format("%s/%s +set com_automation 1", TODO_TDM_DIR, TDM_NAME);
                long res = wxExecute(cmdline, wxEXEC_ASYNC, nullptr, &env);
                if (res <= 0) {
                    showError("Failed to run TheDarkMod executable.");
                    return -1;
                }

                //attach to the new process
                static const int TDM_LAUNCH_TIMEOUT = 5000;  //in milliseconds
                wxLongLong timestampStart = wxGetUTCTimeMillis();
                do {
                    wxMilliSleep(500);
                    if (wxGetUTCTimeMillis() - timestampStart > TDM_LAUNCH_TIMEOUT) {
                        showError("Timeout when connecting to just started TheDarkMod process.\nMake sure the game is in main menu, has com_automation enabled, and firewall does not block it.");
                        return -1;
                    }
                } while (!connect());
            }

            //check the current status
            std::map<std::string, std::string> statusProps = executeQueryStatus();
            changingFm = false;
            if (statusProps["currentfm"] != TODO_MISSION) {
                //change mission/mod and restart TDM engine
                std::string request = actionPreamble("installfm") + "content:\n" + TODO_MISSION + "\n";
                sendRequest(request);
                _multistepWaitsForSeqno = _seqnoInProgress;
                changingFm = true;
            }

            return 1;
        }

        if (step == 1) {
            std::string response(_response.begin(), _response.end());
            if (changingFm && response != "done") {
                showError("Failed to change installed mission in TheDarkMod.\nMake sure ?DR mission? is configured properly and game version is 2.09 or above.");
                return -1;
            }
            std::map<std::string, std::string> statusProps = executeQueryStatus();
            if (statusProps["currentfm"] != TODO_MISSION) {
                showError(fmt::format("Installed mission is {} despite trying to change it.", statusProps["currentfm"]));
                return -1;
            }

            if (dmap) {
                //run dmap command
                std::string request = composeConExecRequest("dmap " + std::string(TODO_MAP));
                sendRequest(request);
                _multistepWaitsForSeqno = _seqnoInProgress;
            }

            return 2;
        }

        if (step == 2) {
            if (dmap) {
                std::string response(_response.begin(), _response.end());
                if (response.find("ERROR:") != std::string::npos) {
                    showError("Dmap printed error.\nPlease look at TheDarkMod console.");
                    return -1;
                }
            }

            //start map
            std::string request = composeConExecRequest("map " + std::string(TODO_MAP));
            sendRequest(request);
            _multistepWaitsForSeqno = _seqnoInProgress;

            return 3;
        }

        if (step == 3) {
            std::string response(_response.begin(), _response.end());

            std::map<std::string, std::string> statusProps = executeQueryStatus();
            if (statusProps["currentfm"] != TODO_MISSION) {
                showError(fmt::format("Installed mission is still {}.", statusProps["currentfm"]));
                return -1;
            }
            if (statusProps["mapname"] != TODO_MAP) {
                showError(fmt::format("Active map is {} despite trying to start the map.", statusProps["mapname"]));
                return -1;
            }
            if (statusProps["guiactive"] != "") {
                showError(fmt::format("GUI {} is active while we expect the game to start", statusProps["guiactive"]));
                return -1;
            }

            //confirm player is ready
            std::string waitUntilReady = executeGetCvarValue("tdm_player_wait_until_ready");
            if (waitUntilReady != "0") {
                //button0 is "attack" button
                //numbers in parens mean: hold for 100 gameplay milliseconds (time is stopped at waiting screen)
                std::string request = actionPreamble("gamectrl") + "content:\n" + "timemode \"game\"\n" + "button0 (1 1 0 0 0 0.1)\n";
                std::string response = executeRequest(request);
            }

            if (!savedViewPos.empty()) {
                //restore camera position
                std::string request = composeConExecRequest(fmt::format("setviewpos {}", savedViewPos));
                std::string response = executeRequest(request);
            }

            return -1;
        }
    };

    _multistepProcedureFunction = implementation;
    _multistepProcedureStep = 0;
    continueMultistepProcedure();*/
}

//-------------------------------------------------------------

std::string GameConnection::composeConExecRequest(std::string consoleLine) {
    //remove trailing spaces/EOLs
    while (!consoleLine.empty() && isspace(consoleLine.back()))
        consoleLine.pop_back();
    return actionPreamble("conexec") + "content:\n" + consoleLine + "\n";
}

void GameConnection::executeSetTogglableFlag(const std::string &toggleCommand, bool enable, const std::string &offKeyword) {
    std::string text = composeConExecRequest(toggleCommand);
    int attempt;
    for (attempt = 0; attempt < 2; attempt++) {
        std::string response = executeGenericRequest(text);
        bool isEnabled = (response.find(offKeyword) == std::string::npos);
        if (enable == isEnabled)
            break;
        //wrong state: toggle it again
    }
    assert(attempt < 2);    //two toggles not enough?...
}

std::string GameConnection::executeGetCvarValue(const std::string &cvarName, std::string *defaultValue) {
    std::string text = composeConExecRequest(cvarName);
    std::string response = executeGenericRequest(text);

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
    std::string response = executeGenericRequest(request);

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
    try {
        auto& camera = GlobalCameraManager().getActiveView();

        Vector3 orig = camera.getCameraOrigin();
        Vector3 angles = camera.getCameraAngles();

        _cameraOutData[0] = orig;
        _cameraOutData[1] = angles;
        //note: the update is not necessarily sent right now
        _cameraOutPending = true;
    }
    catch (const std::runtime_error&) {
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

        _engine->executeRequestAsync(TAG_CAMERA, text);
        _cameraOutPending = false;

        return true;
    }
    return false;
}

bool GameConnection::setCameraSyncEnabled(bool enable)
{
    try {
        if (!enable) {
            _cameraChangedSignal.disconnect();
        }
        if (enable) {
            _cameraChangedSignal.disconnect();
            _cameraChangedSignal = GlobalCameraManager().signal_cameraChanged().connect(
                sigc::mem_fun(this, &GameConnection::updateCamera)
            );

            executeSetTogglableFlag("god", true, "OFF");
            executeSetTogglableFlag("noclip", true, "OFF");
            executeSetTogglableFlag("notarget", true, "OFF");

            //sync camera location right now
            updateCamera();
            _engine->waitForTags(1 << TAG_CAMERA);
        }
    }
    catch (const DisconnectException&) {
        //disconnected: will be handled during next think
        return false;
    }
    return true;
}

void GameConnection::backSyncCamera()
{
    try {
        _engine->waitForTags();
        std::string text = executeGenericRequest(composeConExecRequest("getviewpos"));

        Vector3 orig, angles;
        if (sscanf(text.c_str(), "%lf%lf%lf%lf%lf%lf", &orig.x(), &orig.y(), &orig.z(), &angles.x(), &angles.y(), &angles.z()) == 6) {
            try {
                auto& camera = GlobalCameraManager().getActiveView();
                angles.x() *= -1.0;
                camera.setOriginAndAngles(orig, angles);
            }
            catch (const std::runtime_error&) {
                // no camera view
            }
        }
    }
    catch (const DisconnectException&) {
        //disconnected: will be handled during next think
        return;
    }
}

void GameConnection::togglePauseGame()
{
    try {
        std::string value = executeGetCvarValue("g_stopTime");
        std::string oppositeValue = (value == "0" ? "1" : "0");
        std::string text = composeConExecRequest(fmt::format("g_stopTime {}", oppositeValue));
        executeGenericRequest(text);
    }
    catch (const DisconnectException&) {
        //disconnected: will be handled during next think
        return;
    }
}

void GameConnection::respawnSelectedEntities()
{
    try {
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
        executeGenericRequest(text);
    }
    catch (const DisconnectException&) {
        //disconnected: will be handled during next think
        return;
    }
}

void GameConnection::reloadMap()
{
    try {
        std::string text = composeConExecRequest("reloadMap nocheck");
        executeGenericRequest(text);

        if (!GlobalMapModule().isModified()) {
            //TDM has reloaded .map file while we have no local changes
            //it means all three version of the map are in sync
            //so we can enable "update map" mode without fear for confusion
            setUpdateMapObserverEnabled(true);
        }
        else {
            //TDM has reloaded .map file but we have local changes
            //next "update map" will send diff based on our modified version
            //but it will be applied on top of TDM's state (which is different)
            //better disable the mode to avoid confusion!
            setUpdateMapObserverEnabled(false);
        }
    }
    catch (const DisconnectException&) {
        //disconnected: will be handled during next think
        return;
    }
}

void GameConnection::onMapEvent(IMap::MapEvent ev)
{
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
    if (enable && !_engine->isAlive())
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

bool GameConnection::setUpdateMapAlways(bool enable)
{
    if (enable) {
        if (!_engine->isAlive())
            return false;
        if (enable)
            setUpdateMapObserverEnabled(true);
    }

    _updateMapAlways = enable;
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
    try {
        if (!_engine->isAlive())
            return; //no connection, don't even try

        // Get map diff
        std::string diff = saveMapDiff(_mapObserver.getChanges());
        if (diff.empty()) {
            return; //TODO: fail
        }

        std::string response = executeGenericRequest(actionPreamble("reloadmap-diff") + "content:\n" + diff);
        if (response.find("HotReload: SUCCESS") != std::string::npos) {
            //success: clear current diff, so that we don't reapply it next time
            _mapObserver.clear();
        }
    }
    catch (const DisconnectException&) {
        //disconnected: will be handled during next think
        return;
    }
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
