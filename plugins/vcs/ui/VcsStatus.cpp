#include "VcsStatus.h"

#include "i18n.h"
#include "imap.h"
#include "iuserinterface.h"
#include "imainframe.h"

#include <wx/sizer.h>
#include <sigc++/functors/mem_fun.h>

#include "registry/registry.h"
#include "os/file.h"
#include "os/path.h"
#include "../GitModule.h"

namespace vcs
{

namespace ui
{

VcsStatus::VcsStatus(wxWindow* parent) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER | wxWANTS_CHARS, "VcsStatusBarPanel"),
    _timer(this),
    _fetchInProgress(false)
{
    SetSizer(new wxBoxSizer(wxHORIZONTAL));

    _mapStatus = new wxStaticText(this, wxID_ANY, "");
    GetSizer()->Add(_mapStatus, wxRIGHT, 6);

    _text = new wxStaticText(this, wxID_ANY, _("Not under Version Control"));
    GetSizer()->Add(_text);

    Bind(wxEVT_TIMER, &VcsStatus::onIntervalReached, this);
    Bind(wxEVT_IDLE, &VcsStatus::onIdle, this);

    GlobalRegistry().signalForKey(RKEY_AUTO_FETCH_ENABLED).connect(
        sigc::mem_fun(this, &VcsStatus::restartTimer)
    );
    GlobalRegistry().signalForKey(RKEY_AUTO_FETCH_INTERVAL).connect(
        sigc::mem_fun(this, &VcsStatus::restartTimer)
    );

    GlobalMapModule().signal_modifiedChanged().connect(
        sigc::mem_fun(this, &VcsStatus::updateMapFileStatus)
    );

    GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(this, &VcsStatus::onMapEvent)
    );
}

VcsStatus::~VcsStatus()
{
    if (_fetchTask.valid())
    {
        _fetchTask.get(); // Wait for the thread to complete
    }

    if (_mapFileTask.valid())
    {
        _mapFileTask.get(); // Wait for the thread to complete
    }
}

void VcsStatus::setRepository(const std::shared_ptr<git::Repository>& repository)
{
    _repository = repository;

    if (!_repository)
    {
        _text->SetLabel(_("Not under version control"));
        _timer.Stop();
        return;
    }

    _text->SetLabel(_repository->getCurrentBranchName());
    restartTimer();
}

void VcsStatus::restartTimer()
{
    _timer.Stop();

    if (registry::getValue<bool>(RKEY_AUTO_FETCH_ENABLED))
    {
        int interval = static_cast<int>(registry::getValue<float>(RKEY_AUTO_FETCH_INTERVAL) * 60 * 1000);

        if (interval > 0)
        {
            _timer.Start(interval);
        }
    }
}

void VcsStatus::onMapEvent(IMap::MapEvent ev)
{
    if (ev == IMap::MapSaved)
    {
        updateMapFileStatus();
    }
}

void VcsStatus::onIntervalReached(wxTimerEvent& ev)
{
    std::lock_guard<std::mutex> guard(_taskLock);

    if (_fetchInProgress) return;

    if (!GlobalMainFrame().isActiveApp())
    {
        rMessage() << "Skipping fetch this round, since the app is not active." << std::endl;
        return;
    }

    _fetchInProgress = true;
    auto repository = _repository->clone();
    _fetchTask = std::async(std::launch::async, std::bind(&VcsStatus::performFetch, this, repository));
}

void VcsStatus::updateMapFileStatus()
{
    if (GlobalMapModule().isModified())
    {
        _mapStatus->SetLabel(_("Map is modified"));
    }
    else
    {
        _mapStatus->SetLabel(_("Map is saved"));

        if (_repository)
        {
            auto repository = _repository->clone();
            _mapFileTask = std::async(std::launch::async, std::bind(&VcsStatus::performMapFileStatusCheck, this, repository));
        }
    }
}

void VcsStatus::onIdle(wxIdleEvent& ev)
{
    ev.Skip();
}

void VcsStatus::performFetch(std::shared_ptr<git::Repository> repository)
{
    GlobalUserInterface().dispatch([this]() { _text->SetLabel(_("Fetching...")); });

    repository->fetchFromTrackedRemote();

    std::lock_guard<std::mutex> guard(_taskLock);
    _fetchInProgress = false;

    auto status = repository->isUpToDateWithRemote() ? _("Up to date") : _("Updates available");
    GlobalUserInterface().dispatch([this, status]() { _text->SetLabel(status); });
}

void VcsStatus::setMapFileStatus(const std::string& status)
{
    GlobalUserInterface().dispatch([this, status]() { _mapStatus->SetLabel(status); });
}

void VcsStatus::performMapFileStatusCheck(std::shared_ptr<git::Repository> repository)
{
    setMapFileStatus(_("Checking map status..."));

    if (GlobalMapModule().isUnnamed())
    {
        setMapFileStatus(_("Map not saved yet"));
        return;
    }

    auto mapName = GlobalMapModule().getMapName();

    if (!os::fileOrDirExists(mapName))
    {
        setMapFileStatus(_("Unknown"));
        return;
    }

    auto relativePath = os::getRelativePath(mapName, repository->getPath());

    if (relativePath == mapName)
    {
        setMapFileStatus(_("Map outside VCS"));
        return;
    }

    if (repository->fileHasUncommittedChanges(relativePath))
    {
        setMapFileStatus(_("Map saved, pending commit"));
    }
    else
    {
    setMapFileStatus(_("Map committed"));
    }
}

}

}
