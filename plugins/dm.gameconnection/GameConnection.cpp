#include "GameConnection.h"
#include "DiffStatus.h"
#include "DiffDoom3MapWriter.h"
#include "AutomationEngine.h"

#include "i18n.h"
#include "igame.h"
#include "icameraview.h"
#include "inode.h"
#include "imap.h"
#include "ientity.h"
#include "iselection.h"
#include "ui/iuserinterface.h"
#include "ui/imenumanager.h"
#include "ui/imainframe.h"

#include "scene/Traverse.h"
#include "wxutil/Bitmap.h"
#include "util/ScopedBoolLock.h"
#include "registry/registry.h"

#include <sigc++/signal.h>
#include <sigc++/connection.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/process.h>

#include "GameConnectionControl.h"
#include "messages/MapFileOperation.h"
#include "messages/NotificationMessage.h"

namespace gameconn
{

namespace
{
    //this is how often this class "thinks" when idle
    constexpr int THINK_INTERVAL = 123;

    //all ordinary requests, executed synchronously
    constexpr int TAG_GENERIC = 5;
    //camera DR->TDM sync is continuous, executed asynchronously
    constexpr int TAG_CAMERA = 6;
    //multistep procedure for TDM game start/restart
    constexpr int TAG_RESTART = 7;

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

    if (_engine->areTagsInProgress(1 << TAG_RESTART) != _restartInProgress) {
        //restart game procedure has just ended: notify GUI
        _restartInProgress = !_restartInProgress;
        signal_StatusChanged.emit(0);
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

void GameConnection::setThinkLoop(bool enable)
{
    if (enable && !_thinkTimer) {
        _thinkTimer.reset(new wxTimer());
        _thinkTimer->Bind(wxEVT_TIMER, &GameConnection::onTimerEvent, this);
        _thinkTimer->Start(THINK_INTERVAL);
    }
    if (!enable && _thinkTimer) {
        _thinkTimer->Stop();
        _thinkTimer.reset();
    }
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

    setThinkLoop(true);

    _mapEventListener = GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(*this, &GameConnection::onMapEvent)
    );

    signal_StatusChanged.emit(0);

    return true;
}

void GameConnection::disconnect(bool force)
{
    _autoReloadMap = false;
    setAlwaysUpdateMapEnabled(false);
    setUpdateMapObserverEnabled(false);
    setCameraSyncEnabled(false);

    _engine->disconnect(force);
    assert(!_engine->isAlive() && !_engine->hasLostConnection());

    setThinkLoop(false);
    _mapEventListener.disconnect();

    signal_StatusChanged.emit(0);
}

GameConnection::~GameConnection()
{
    disconnect(true);
};

void GameConnection::restartGame(bool dmap)
{
    enum Steps {
        STEP_START,
        STEP_ATTACH,
        STEP_SETMOD,
        STEP_DMAP,
        STEP_SETMAP,
        STEP_INGAME,
        STEP_FINISHED = -1,
    };

    auto implementation = [this, dmap](int step) -> MultistepProcReturn {
        try {
#ifdef _WIN32
            static const char *TDM_EXE_NAME = "TheDarkModx64.exe";
#else
            static const char *TDM_EXE_NAME = "thedarkmod.x64";
#endif

            //perhaps it is not the best idea to store state in global/static variables...
            static std::string tdmDir, drModName, drMapName;
            static std::string savedViewPos;
            static bool savedCameraSyncEnabled, savedAutoReloadMapEnabled, savedAlwaysUpdateMapEnabled;
            static wxLongLong timestampStartAttach, timestampLastTry;
            static int pendingSeqno;

            if (step == STEP_START) {
                //fetch all settings: mission, map, TDM path
                if (GlobalMapModule().isUnnamed())
                {
                    radiant::NotificationMessage::SendError("Cannot start TDM because no map file is opened.");
                    return {STEP_FINISHED, {}};
                }
                tdmDir = os::standardPathWithSlash(registry::getValue<std::string>(RKEY_ENGINE_PATH));
                std::string fullModPath = registry::getValue<std::string>(RKEY_MOD_PATH);
                std::string fullMapPath = GlobalMapModule().getMapName();
                if (!fullModPath.empty() && strchr("/\\", fullModPath.back()))
                    fullModPath.pop_back();
                drModName = fs::path(fullModPath).filename().string();
                drMapName = fs::path(fullMapPath).filename().string();

                //save enabled modes
                savedCameraSyncEnabled = isCameraSyncEnabled();
                savedAutoReloadMapEnabled = isAutoReloadMapEnabled();
                savedAlwaysUpdateMapEnabled = isAlwaysUpdateMapEnabled();
                savedViewPos = "";

                if (isAlive()) {
                    //save current position
                    _engine->waitForTags(TAG_CAMERA);
                    savedViewPos = executeGenericRequest(composeConExecRequest("getviewpos"));
                    //disable modes
                    setCameraSyncEnabled(false);
                    setAutoReloadMapEnabled(false);
                    setAlwaysUpdateMapEnabled(false);
                    setUpdateMapObserverEnabled(false);
                }

                //save .map file
                saveMapIfNeeded();

                //try to attach to TDM with automation enabled
                bool attached = connect();

                if (!attached) {
                    //run new TDM process
                    wxExecuteEnv env;
                    env.cwd = tdmDir;
                    fs::path exeFullPath = fs::path(tdmDir);
                    exeFullPath.concat(TDM_EXE_NAME);
                    wxString cmdline = wxString::Format("%s +set com_automation 1", exeFullPath.c_str());
                    long res = wxExecute(cmdline, wxEXEC_ASYNC, nullptr, &env);
                    if (res <= 0) {
                        radiant::NotificationMessage::SendError("Failed to run TheDarkMod executable.");
                        return {STEP_FINISHED, {}};
                    }

                    timestampStartAttach = wxGetUTCTimeMillis();
                    timestampLastTry = 0;
                    return {STEP_ATTACH, {}};
                }

                return {STEP_SETMOD, {}};
            }

            if (step == STEP_ATTACH) {
                //attach to the new process
                static const int TDM_ATTACH_TIMEOUT = 10000;    //in milliseconds
                static const int TDM_ATTACH_RETRY = 1000;

                wxLongLong timestampNow = wxGetUTCTimeMillis();
                if (timestampNow - timestampLastTry > TDM_ATTACH_RETRY) {
                    timestampLastTry = timestampNow;
                    if (connect())
                        return {STEP_SETMOD, {}};
                }
                if (timestampNow - timestampStartAttach > TDM_ATTACH_TIMEOUT) {
                    radiant::NotificationMessage::SendError("Timeout when connecting to just started TheDarkMod process.\n"
                        "Make sure the game is in main menu, has com_automation enabled, and firewall does not block it.");
                    return {STEP_FINISHED, {}};
                }

                //keep calling this step on every think
                return {STEP_ATTACH, {SEQNO_WAIT_POLL}};
            }

            if (step == STEP_SETMOD) {
                //check the current status
                std::map<std::string, std::string> statusProps = executeQueryStatus();

                if (statusProps["currentfm"] != drModName) {
                    //TDM crashes if we restart engine from in-game or immediately after stopping it
                    //workaround it by giving it a bit of time after stop
                    executeGenericRequest(composeConExecRequest("disconnect"));
                    wxMilliSleep(1000);

                    //change mission/mod and restart TDM engine
                    std::string request = actionPreamble("installfm") + "content:\n" + drModName + "\n";
                    pendingSeqno = _engine->executeRequestAsync(TAG_GENERIC, request);
                    return {STEP_DMAP, {pendingSeqno}};
                }

                pendingSeqno = 0;
                return {STEP_DMAP, {}};
            }

            if (step == STEP_DMAP) {
                //handle response of mission/mod request
                if (pendingSeqno) {
                    std::string response = _engine->getResponse(pendingSeqno);
                    if (response != "done") {
                        radiant::NotificationMessage::SendError("Failed to change installed mission in TheDarkMod.\n"
                            "Make sure ?DR mission? is configured properly and game version is 2.09 or above.");
                        return {STEP_FINISHED, {}};
                    }
                }
                //recheck currently installed FM just to be sure
                std::map<std::string, std::string> statusProps = executeQueryStatus();
                if (statusProps["currentfm"] != drModName) {
                    radiant::NotificationMessage::SendError(fmt::format("Installed mission is {} despite trying to change it.", statusProps["currentfm"]));
                    return {STEP_FINISHED, {}};
                }

                if (dmap) {
                    //run dmap command
                    std::string request = composeConExecRequest("dmap " + drMapName);
                    pendingSeqno = _engine->executeRequestAsync(TAG_GENERIC, request);
                    return {STEP_SETMAP, {pendingSeqno}};
                }
                pendingSeqno = 0;
                return {STEP_SETMAP, {}};
            }

            if (step == STEP_SETMAP) {
                //handle response of dmap command
                if (pendingSeqno) {
                    std::string response = _engine->getResponse(pendingSeqno);
                    if (response.find("ERROR:") != std::string::npos) {
                        radiant::NotificationMessage::SendError("Dmap printed error.\nPlease look at TheDarkMod console.");
                        return {STEP_FINISHED, {}};
                    }
                }

                //start map
                std::string request = composeConExecRequest("map " + drMapName);
                pendingSeqno = _engine->executeRequestAsync(TAG_GENERIC, request);

                return {STEP_INGAME, {pendingSeqno}};
            }

            if (step == STEP_INGAME) {
                //don't care what was written to console while loading map =)
                std::string response = _engine->getResponse(pendingSeqno);

                //last check: everything should match!
                std::map<std::string, std::string> statusProps = executeQueryStatus();
                if (statusProps["currentfm"] != drModName) {
                    radiant::NotificationMessage::SendError(fmt::format("Installed mission is still {}.", statusProps["currentfm"]));
                    return {STEP_FINISHED, {}};
                }
                if (statusProps["mapname"] != drMapName) {
                    radiant::NotificationMessage::SendError(fmt::format("Active map is {} despite trying to start the map.", statusProps["mapname"]));
                    return {STEP_FINISHED, {}};
                }
                if (statusProps["guiactive"] != "") {
                    radiant::NotificationMessage::SendError(fmt::format("GUI {} is active while we expect the game to start", statusProps["guiactive"]));
                    return {STEP_FINISHED, {}};
                }

                //confirm player is ready
                std::string waitUntilReady = executeGetCvarValue("tdm_player_wait_until_ready");
                if (waitUntilReady != "0") {
                    //button0 is "attack" button
                    //numbers in parens mean: hold for 100 gameplay milliseconds (time is stopped at waiting screen)
                    std::string request = actionPreamble("gamectrl") + "content:\n" + "timemode \"game\"\n" + "button0 (1 1 0 0 0 0.1)\n";
                    response = executeGenericRequest(request);
                }

                if (!savedViewPos.empty()) {
                    //force god/noclip/notarget on
                    enableGhostMode();
                    //restore camera position
                    std::string request = composeConExecRequest(fmt::format("setviewpos {}", savedViewPos));
                    response = executeGenericRequest(request);
                }

                //enable back all the modes
                setCameraSyncEnabled(savedCameraSyncEnabled);
                setAutoReloadMapEnabled(savedAutoReloadMapEnabled);
                //if user has changed something, don't enable "update map" mode to avoid desync
                if (!GlobalMapModule().isModified()) {
                    setUpdateMapObserverEnabled(true);
                    setAlwaysUpdateMapEnabled(savedAlwaysUpdateMapEnabled);
                }

                return {STEP_FINISHED, {}};
            }
        }
        catch(const DisconnectException&) {
            //connection was lost unexpectedly during some step
            //most likely user has closed TDM while it was still being prepared
            radiant::NotificationMessage::SendError("Game restart failed: connection lost unexpectedly.");
        }
        return {STEP_FINISHED, {}};
    };

    _engine->executeMultistepProc(TAG_RESTART, implementation, STEP_START);

    _restartInProgress = true;
    signal_StatusChanged.emit(0);

    //even if TDM is not yet started, we need our engine's think loop
    //because otherwise this multistep procedure won't start
    setThinkLoop(true);
}

bool GameConnection::isGameRestarting() const
{
    return _restartInProgress;
}

//-------------------------------------------------------------

std::string GameConnection::composeConExecRequest(std::string consoleLine)
{
    //remove trailing spaces/EOLs
    while (!consoleLine.empty() && isspace(consoleLine.back()))
        consoleLine.pop_back();
    return actionPreamble("conexec") + "content:\n" + consoleLine + "\n";
}

void GameConnection::executeSetTogglableFlag(const std::string &toggleCommand, bool enable, const std::string &offKeyword)
{
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

std::string GameConnection::executeGetCvarValue(const std::string &cvarName, std::string *defaultValue)
{
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

std::map<std::string, std::string> GameConnection::executeQueryStatus()
{
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

void GameConnection::updateCamera()
{
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

bool GameConnection::sendPendingCameraUpdate()
{
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

void GameConnection::enableGhostMode()
{
    executeSetTogglableFlag("god", true, "OFF");
    executeSetTogglableFlag("noclip", true, "OFF");
    executeSetTogglableFlag("notarget", true, "OFF");
}

void GameConnection::setCameraSyncEnabled(bool enable)
{
    try {
        if (!enable) {
            _cameraChangedSignal.disconnect();
        }
        if (enable) {
            enableGhostMode();

            _cameraChangedSignal.disconnect();
            _cameraChangedSignal = GlobalCameraManager().signal_cameraChanged().connect(
                sigc::mem_fun(this, &GameConnection::updateCamera)
            );

            //sync camera location right now
            updateCamera();
            _engine->waitForTags(1 << TAG_CAMERA);
        }

        signal_StatusChanged.emit(0);
    }
    catch (const DisconnectException&) {
        //disconnected: will be handled during next think
    }
}

bool GameConnection::isCameraSyncEnabled() const
{
    return !_cameraChangedSignal.empty();
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

void GameConnection::setAutoReloadMapEnabled(bool enable)
{
    if (enable && !_engine->isAlive())
        return;

    _autoReloadMap = enable;
    signal_StatusChanged.emit(0);
}

bool GameConnection::isAutoReloadMapEnabled() const
{
    return _autoReloadMap;
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

void GameConnection::setUpdateMapObserverEnabled(bool enable)
{
    _mapObserver.setEnabled(enable);
    if (!enable)
        setAlwaysUpdateMapEnabled(false);

    signal_StatusChanged.emit(0);
}

void GameConnection::setAlwaysUpdateMapEnabled(bool enable)
{
    if (enable) {
        if (!_engine->isAlive())
            return;
        if (enable)
            setUpdateMapObserverEnabled(true);
    }

    _updateMapAlways = enable;
    signal_StatusChanged.emit(0);
}

bool GameConnection::isAlwaysUpdateMapEnabled() const
{
    return _updateMapAlways;
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

        // Hack: disable recalculateBrushWindings for this export
        registry::ScopedKeyChanger<std::string> guard("MapExporter_IgnoreBrushes", "yes");

        // Get a scoped exporter class
        auto exporter = GlobalMapModule().createMapExporter(writer, root, outStream);

        try
        {
            // Pass the traversal function and the root of the subgraph to export
            exporter->exportMap(root, scene::traverseSubset(subsetNodes));
            // end the life of the exporter instance here to finish the scene
            exporter.reset();
        }
        catch (map::FileOperation::OperationCancelled&)
        {
            radiant::NotificationMessage::SendInformation(_("Map export cancelled"));
        }
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
        MODULE_MENUMANAGER, MODULE_USERINTERFACE, MODULE_MAINFRAME
    };
    return _dependencies;
}

void GameConnection::initialiseModule(const IApplicationContext& ctx)
{
    // Don't add any commands if the hot_reload feature is not enabled by the current Game
    if (!GlobalGameManager().currentGame()->hasFeature("hot_reload"))
        return;

    GlobalMenuManager().add("main/map", "GameConnectionPanel", ui::menu::ItemType::Item, _("Game Connection..."),
        "", fmt::format("{0}{1}", ui::TOGGLE_CONTROL_STATEMENT_PREFIX, ui::GameConnectionControl::Name));

    GlobalUserInterface().registerControl(std::make_shared<ui::GameConnectionControl>());

    // Add to mainframe after startup, set control default location
    GlobalMainFrame().signal_MainFrameConstructed().connect([&]()
    {
        GlobalMainFrame().addControl(ui::GameConnectionControl::Name, { IMainFrame::Location::FloatingWindow, false });
    });

    // Restart game
    GlobalCommandSystem().addCommand(
        "GameConnectionRestartGame",
        [this](const cmd::ArgumentList& args)
        {
            bool dmap = false;
            for (int i = 0; i < args.size(); i++)
                if (args[i].getString() == "dmap")
                    dmap = true;
            restartGame(dmap);
        }
    );

    // Camera sync
    _event_toggleCameraSync = GlobalEventManager().addAdvancedToggle(
        "GameConnectionToggleCameraSync",
        [this](bool v) {
            bool oldEnabled = isCameraSyncEnabled();
            setCameraSyncEnabled(v);
            return isCameraSyncEnabled() != oldEnabled;
        }
    );
    GlobalCommandSystem().addCommand("GameConnectionBackSyncCamera",
                                     [this](const cmd::ArgumentList&) { backSyncCamera(); });
    _event_backSyncCamera = GlobalEventManager().addCommand(
        "GameConnectionBackSyncCamera", "GameConnectionBackSyncCamera", false
    );

    // Reload map
    GlobalCommandSystem().addCommand("GameConnectionReloadMap",
                                     [this](const cmd::ArgumentList&) { reloadMap(); });
    GlobalEventManager().addAdvancedToggle(
        "GameConnectionToggleAutoMapReload",
        [this](bool v) {
            bool oldEnabled = isAutoReloadMapEnabled();
            setAutoReloadMapEnabled(v);
            return isAutoReloadMapEnabled() != oldEnabled;
        }
    );

    // Update map
    GlobalCommandSystem().addCommand("GameConnectionUpdateMap",
                                     [this](const cmd::ArgumentList&) { doUpdateMap(); });
    GlobalEventManager().addAdvancedToggle(
        "GameConnectionToggleAutoMapUpdate",
        [this](bool v) {
            bool oldEnabled = isAlwaysUpdateMapEnabled();
            setAlwaysUpdateMapEnabled(v);
            return isAlwaysUpdateMapEnabled() != oldEnabled;

        }
    );

    // Respawn selected
    GlobalCommandSystem().addCommand(
        "GameConnectionRespawnSelected",
        [this](const cmd::ArgumentList&) { respawnSelectedEntities(); }
    );

    // Pause game
    GlobalCommandSystem().addCommand("GameConnectionPauseGame",
                                     [this](const cmd::ArgumentList&) { togglePauseGame(); });

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
        _event_toggleCameraSync->connectToolItem(camSyncT);
        auto camSyncBackT = camTB->AddTool(
            wxID_ANY, "B", wxutil::GetLocalBitmap("CameraSyncBack.png"),
            _("Move camera to current game position")
        );
        _event_backSyncCamera->connectToolItem(camSyncBackT);

        camTB->Realize();
    }
}

}

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
    module::performDefaultInitialisation(registry);
    registry.registerModule(std::make_shared<gameconn::GameConnection>());
}
