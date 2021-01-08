#pragma once

#include "imodule.h"
#include "ifilesystem.h"

#include <wx/event.h>
#include <wx/fswatcher.h>
#include <wx/timer.h>
#include <memory>

namespace ui
{
class AutoReloadManager :
    public wxEvtHandler,
    public RegisterableModule,
    public vfs::VirtualFileSystem::Observer
{
public:
    AutoReloadManager();

    // RegisterableModule interface
    const std::string &getName() const override;
    const StringSet &getDependencies() const override;
    void initialiseModule(const IApplicationContext &ctx) override;
    void shutdownModule() override;

    // Observer interface
    void onFileSystemInitialise() override;
    void onFileSystemShutdown() override;

protected:
    void UpdateAutoReloadPath();
    void onDirChanged(wxFileSystemWatcherEvent &ev);
    void onTimerIntervalReached(wxTimerEvent &ev);

private:
    size_t _watchDepth;
    wxFileSystemWatcher _modelWatcher;
    wxTimer _updateTimer;

    bool _modelsNeedUpdate;
    bool _materialsNeedUpdate;
    bool _skinsNeedUpdate;

};

// The accessor function for the static AutoSaver instance
AutoReloadManager& AutoReloader();

const char* const MODULE_AUTORELOAD = "AutoReloadManager";

} // namespace ui
