#include "VcsStatus.h"

#include "i18n.h"
#include "imap.h"
#include "iuserinterface.h"
#include "imainframe.h"

#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <sigc++/functors/mem_fun.h>

#include "registry/registry.h"
#include "os/file.h"
#include "os/path.h"
#include "string/convert.h"
#include "wxutil/menu/CommandMenuItem.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "../GitModule.h"
#include "../Diff.h"
#include "../GitException.h"

namespace vcs
{

namespace ui
{

VcsStatus::VcsStatus(wxWindow* parent) :
    _panel(loadNamedPanel(parent, "VcsStatusBar")),
    _timer(this),
    _fetchInProgress(false),
    _popupMenu(new wxutil::PopupMenu)
{
    _popupMenu->addItem(std::make_shared<wxutil::CommandMenuItem>(
        new wxMenuItem(nullptr, wxID_ANY, _("Check for Changes"), "Load"),
        "GitFetch",
        [this]() { return !_fetchInProgress; }
    ));

    _mapStatus = findNamedObject<wxStaticText>(_panel, "MapStatusLabel");
    _remoteStatus = findNamedObject<wxStaticText>(_panel, "RemoteStatusLabel");
    
    auto* vcsMenu = findNamedObject<wxBitmapButton>(_panel, "VcsMenuButton");
    vcsMenu->Hide();
    vcsMenu->Bind(wxEVT_BUTTON, [this] (wxCommandEvent& ev)
    {
        _popupMenu->show(wxDynamicCast(ev.GetEventObject(), wxWindow));
    });

    Bind(wxEVT_TIMER, &VcsStatus::onIntervalReached, this);
    _panel->Bind(wxEVT_IDLE, &VcsStatus::onIdle, this);

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

    _panel->Destroy();
}

wxWindow* VcsStatus::getWidget()
{
    return _panel;
}

void VcsStatus::setRepository(const std::shared_ptr<git::Repository>& repository)
{
    _repository = repository;

    findNamedObject<wxBitmapButton>(_panel, "VcsMenuButton")->Show(_repository != nullptr);

    if (!_repository)
    {
        _remoteStatus->SetLabel(_("Not under version control"));
        _timer.Stop();
        return;
    }

    _remoteStatus->SetLabel(_repository->getCurrentBranchName());
    restartTimer();

    // Run a fetch update right after connecting to the repo, if auto-fetch is enabled
    if (registry::getValue<bool>(RKEY_AUTO_FETCH_ENABLED))
    {
        startFetchTask();
    }
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

void VcsStatus::startFetchTask()
{
    {
        std::lock_guard<std::mutex> guard(_taskLock);

        if (_fetchInProgress || !_repository) return;

        if (!GlobalMainFrame().isActiveApp())
        {
            rMessage() << "Skipping fetch this round, since the app is not active." << std::endl;
            return;
        }

        _fetchInProgress = true;
    }

    auto repository = _repository->clone();
    _fetchTask = std::async(std::launch::async, std::bind(&VcsStatus::performFetch, this, repository));
}

void VcsStatus::onIntervalReached(wxTimerEvent& ev)
{
    startFetchTask();
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
    auto head = repository->getHead();

    if (!head)
    {
        _fetchInProgress = false;
        return;
    }

    try
    {
        repository->getUpstreamRemoteName(*head);
    }
    catch (const git::GitException&)
    {
        setRemoteStatus(git::RemoteStatus{ 0, 0, _("Not connected") });
        _fetchInProgress = false;
        return;
    }

    try
    {
        setRemoteStatus(git::RemoteStatus{ 0, 0, _("Fetching...") });

        repository->fetchFromTrackedRemote();

        setRemoteStatus(git::analyseRemoteStatus(repository));
    }
    catch (const git::GitException& ex)
    {
        setRemoteStatus(git::RemoteStatus{ 0, 0, ex.what() });
    }

    _fetchInProgress = false;
}

void VcsStatus::setMapFileStatus(const std::string& status)
{
    GlobalUserInterface().dispatch([this, status]() { _mapStatus->SetLabel(status); });
}

void VcsStatus::setRemoteStatus(const git::RemoteStatus& status)
{
    GlobalUserInterface().dispatch([this, status]() 
    { 
        findNamedObject<wxWindow>(_panel, "OutgoingCommitsIcon")->Show(status.localAheadCount > 0);
        findNamedObject<wxWindow>(_panel, "IncomingCommitsIcon")->Show(status.remoteAheadCount > 0);
        
        auto* outgoingLabel = findNamedObject<wxStaticText>(_panel, "NumOutgoingCommits");
        outgoingLabel->Show(status.localAheadCount > 0);
        outgoingLabel->SetLabel(string::to_string(status.localAheadCount));

        auto* incomingLabel = findNamedObject<wxStaticText>(_panel, "NumIncomingCommits");
        incomingLabel->Show(status.remoteAheadCount > 0);
        incomingLabel->SetLabel(string::to_string(status.remoteAheadCount));

        _remoteStatus->SetLabel(status.label); 
    });
}

void VcsStatus::performMapFileStatusCheck(std::shared_ptr<git::Repository> repository)
{
    setMapFileStatus(_("Checking map status..."));

    try
    {
        if (GlobalMapModule().isUnnamed())
        {
            setMapFileStatus(_("Map not saved yet"));
            return;
        }

        auto relativePath = repository->getRepositoryRelativePath(GlobalMapModule().getMapName());

        if (relativePath.empty())
        {
            setMapFileStatus(_("Map not in VCS"));
            return;
        }

        if (repository->fileHasUncommittedChanges(relativePath))
        {
            setMapFileStatus(_("Map saved, pending commit"));
        }
        else if (repository->fileIsIndexed(relativePath))
        {
            setMapFileStatus(_("Map committed"));
        }
        else
        {
            setMapFileStatus(_("Map saved"));
        }
    }
    catch (const git::GitException& ex)
    {
        setMapFileStatus(std::string("ERROR: ") + ex.what());
    }
}

}

}
