#include "AutoReloadManager.h"
#include "igame.h"
#include "icommandsystem.h"
#include "module/StaticModule.h"

#include <sigc++/functors/mem_fun.h>

namespace ui
{

module::StaticModule<AutoReloadManager> staticAutoReloadModule;

AutoReloadManager::AutoReloadManager()
    : _modelsNeedUpdate(false),
      _materialsNeedUpdate(false),
      _skinsNeedUpdate(false)
{
}

void AutoReloadManager::UpdateAutoReloadPath()
{
    //Get the FM directory, and cache the depth of its top-level subdirs
    wxFileName dirPath(GlobalGameManager().getModPath());
    _watchDepth = dirPath.GetDirCount();

    //Start watching the FM directory
    _modelWatcher.RemoveAll();
    if (_modelWatcher.AddTree(dirPath, wxFSW_EVENT_CREATE | wxFSW_EVENT_DELETE | wxFSW_EVENT_RENAME | wxFSW_EVENT_MODIFY))
        rMessage() << "Started watching " << dirPath.GetFullPath() << std::endl;
    else
        rMessage() << "Could not watch " << dirPath.GetFullPath() << std::endl;
}

void AutoReloadManager::onDirChanged(wxFileSystemWatcherEvent &ev)
{
    //If the notification was from a top-level subdir, figure out which one it was
    if (ev.GetPath().GetDirCount() > _watchDepth)
    {
        const wxString &dirPart = ev.GetPath().GetDirs().Item(_watchDepth);

        //Take the approprate action based on which directory emitted the notification.
        //We delay the action with a timer because some filesystem events generate redundant notifications.
        if (dirPart == "models")
        {
            _modelsNeedUpdate = true;
            _updateTimer.StartOnce(100);
        }
        else if (dirPart == "materials")
        {
            _materialsNeedUpdate = true;
            _updateTimer.StartOnce(100);
        }
        else if (dirPart == "skins")
        {
            _skinsNeedUpdate = true;
            _updateTimer.StartOnce(100);
        }
    }
}

void AutoReloadManager::onTimerIntervalReached(wxTimerEvent &ev)
{
    if (_modelsNeedUpdate)
    {
        rMessage() << "models directory changed, reloading..." << std::endl;
        GlobalCommandSystem().executeCommand("RefreshModels");
        _modelsNeedUpdate = false;
    }
    if (_materialsNeedUpdate)
    {
        rMessage() << "materials directory changed, reloading..." << std::endl;
        GlobalCommandSystem().executeCommand("RefreshShaders");
        _materialsNeedUpdate = false;
    }
    if (_skinsNeedUpdate)
    {
        rMessage() << "skins directory changed, reloading..." << std::endl;
        GlobalCommandSystem().executeCommand("ReloadSkins");
        _skinsNeedUpdate = false;
    }
}

const std::string &AutoReloadManager::getName() const
{
    static std::string _name(MODULE_AUTORELOAD);
    return _name;
}

const StringSet &AutoReloadManager::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_COMMANDSYSTEM);
        _dependencies.insert(MODULE_GAMEMANAGER);
        _dependencies.insert(MODULE_VIRTUALFILESYSTEM);
    }

    return _dependencies;
}

void ui::AutoReloadManager::initialiseModule(const IApplicationContext &ctx)
{
    _modelWatcher.SetOwner(this);
    Connect(wxEVT_FSWATCHER, wxFileSystemWatcherEventHandler(AutoReloadManager::onDirChanged), nullptr, this);

    _updateTimer.SetOwner(this);
    Connect(wxEVT_TIMER, wxTimerEventHandler(AutoReloadManager::onTimerIntervalReached), nullptr, this);

    GlobalFileSystem().addObserver(*this);
    UpdateAutoReloadPath();
}

void ui::AutoReloadManager::shutdownModule()
{
    _modelWatcher.RemoveAll();
    Disconnect(wxID_ANY, wxEVT_FSWATCHER);
    Disconnect(wxID_ANY, wxEVT_TIMER);
}

void AutoReloadManager::onFileSystemInitialise()
{
    UpdateAutoReloadPath();
}

void AutoReloadManager::onFileSystemShutdown()
{
    _modelWatcher.RemoveAll();
}

}
