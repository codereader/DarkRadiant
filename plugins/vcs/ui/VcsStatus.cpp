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
#include "wxutil/dialog/MessageBox.h"
#include "util/ScopedBoolLock.h"
#include "CommitDialog.h"
#include "../GitModule.h"
#include "../Diff.h"
#include "../GitException.h"
#include "../Algorithm.h"

namespace vcs
{

namespace ui
{

VcsStatus::VcsStatus(wxWindow* parent) :
    _panel(loadNamedPanel(parent, "VcsStatusBar")),
    _fetchTimer(this),
    _statusTimer(this),
    _taskInProgress(false),
    _popupMenu(new wxutil::PopupMenu)
{
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
        sigc::mem_fun(this, &VcsStatus::restartFetchTimer)
    );
    GlobalRegistry().signalForKey(RKEY_AUTO_FETCH_INTERVAL).connect(
        sigc::mem_fun(this, &VcsStatus::restartFetchTimer)
    );

    GlobalMapModule().signal_modifiedChanged().connect(
        sigc::mem_fun(this, &VcsStatus::updateMapFileStatus)
    );

    GlobalMapModule().signal_mapEvent().connect(
        sigc::mem_fun(this, &VcsStatus::onMapEvent)
    );

    createPopupMenu();

    _statusTimer.Start(500);
}

VcsStatus::~VcsStatus()
{
    if (_repositoryTask.valid())
    {
        _repositoryTask.get(); // Wait for the thread to complete
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

void VcsStatus::createPopupMenu()
{
    _popupMenu->addItem(std::make_shared<wxutil::MenuItem>(
        new wxMenuItem(nullptr, wxID_ANY, _("Commit"), ""),
        [this]() { performCommit(); },
        [this]() { return canCommit(); }

    ));
    _popupMenu->addItem(std::make_shared<wxutil::CommandMenuItem>(
        new wxMenuItem(nullptr, wxID_ANY, _("Check for Changes"), ""),
        "GitFetch",
        [this]() { return !_taskInProgress; }
    ));

    _popupMenu->addItem(std::make_shared<wxutil::MenuItem>(
        new wxMenuItem(nullptr, wxID_ANY, _("Sync Changes with Server"), ""),
        [this]() { performSync(_repository); },
        [this]() { return canSync(); }
    ));
}

void VcsStatus::setRepository(const std::shared_ptr<git::Repository>& repository)
{
    _repository = repository;

    findNamedObject<wxBitmapButton>(_panel, "VcsMenuButton")->Show(_repository != nullptr);

    if (!_repository)
    {
        _remoteStatus->SetLabel(_("Not under version control"));
        _fetchTimer.Stop();
        return;
    }

    _remoteStatus->SetLabel(_repository->getCurrentBranchName());
    restartFetchTimer();

    // Run a fetch update right after connecting to the repo, if auto-fetch is enabled
    if (registry::getValue<bool>(RKEY_AUTO_FETCH_ENABLED))
    {
        startFetchTask();
    }
}

void VcsStatus::restartFetchTimer()
{
    _fetchTimer.Stop();

    if (registry::getValue<bool>(RKEY_AUTO_FETCH_ENABLED))
    {
        int interval = static_cast<int>(registry::getValue<float>(RKEY_AUTO_FETCH_INTERVAL) * 60 * 1000);

        if (interval > 0)
        {
            _fetchTimer.Start(interval);
        }
    }
}

void VcsStatus::onMapEvent(IMap::MapEvent ev)
{
    if (ev == IMap::MapSaved || ev == IMap::MapLoaded)
    {
        updateMapFileStatus();

        if (_repository)
        {
            analyseRemoteStatus(_repository);
        }
    }
}

void VcsStatus::startFetchTask()
{
    {
        std::lock_guard<std::mutex> guard(_taskLock);

        if (_taskInProgress || !_repository) return;

        if (!GlobalMainFrame().isActiveApp())
        {
            rMessage() << "Skipping fetch this round, since the app is not active." << std::endl;
            return;
        }

        _taskInProgress = true;
    }

    auto repository = _repository->clone();
    _repositoryTask = std::async(std::launch::async, std::bind(&VcsStatus::performFetch, this, repository));
}

void VcsStatus::onIntervalReached(wxTimerEvent& ev)
{
    if (ev.GetTimer().GetId() == _fetchTimer.GetId())
    {
        startFetchTask();
    }
    else if (ev.GetTimer().GetId() == _statusTimer.GetId())
    {
        updateMapFileStatus();
    }
}

void VcsStatus::updateMapFileStatus()
{
    if (GlobalMapModule().isUnnamed())
    {
        setMapFileStatus(_("Map not saved yet"));
        return;
    }

    if (GlobalMapModule().getActiveMergeOperation())
    {
        setMapFileStatus(_("Merging"));
        return;
    }

    if (GlobalMapModule().isModified())
    {
        _mapStatus->SetLabel(_("Map is modified"));
        return;
    }

    _mapStatus->SetLabel(_("Map is saved"));

    if (_repository)
    {
        auto repository = _repository->clone();
        _mapFileTask = std::async(std::launch::async, std::bind(&VcsStatus::performMapFileStatusCheck, this, repository));
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
        _taskInProgress = false;
        return;
    }

    try
    {
        repository->getUpstreamRemoteName(*head);
    }
    catch (const git::GitException&)
    {
        setRemoteStatus(git::RemoteStatus{ 0, 0, _("Not connected") });
        _taskInProgress = false;
        return;
    }

    try
    {
        setRemoteStatus(git::RemoteStatus{ 0, 0, _("Fetching...") });

        repository->fetchFromTrackedRemote();
    }
    catch (const git::GitException& ex)
    {
        setRemoteStatus(git::RemoteStatus{ 0, 0, ex.what() });
    }

    analyseRemoteStatus(repository);

    _taskInProgress = false;
}

void VcsStatus::analyseRemoteStatus(std::shared_ptr<git::Repository> repository)
{
    setRemoteStatus(git::analyseRemoteStatus(repository));
}

void VcsStatus::performSync(std::shared_ptr<git::Repository> repository)
{
    try
    {
        syncWithRemote(repository);
        setRemoteStatus(git::analyseRemoteStatus(repository));
    }
    catch (git::GitException& ex)
    {
        setRemoteStatus(git::RemoteStatus{ 0, 0, ex.what() });
    }
}

bool VcsStatus::canSync()
{
    return !_taskInProgress && _repository && !_repository->mergeIsInProgress();
}

bool VcsStatus::canCommit()
{
    return !_taskInProgress && _repository && !_repository->mergeIsInProgress();
}

void VcsStatus::performCommit()
{
    if (!_repository) return;

    {
        std::lock_guard<std::mutex> guard(_taskLock);

        if (_taskInProgress)
        {
            wxutil::Messagebox::Show(_("Another Task in progress"), 
                _("Cannot commit when another task is in progress"), 
                ::ui::IDialog::MessageType::MESSAGE_CONFIRM);
            return;
        }

        _taskInProgress = true;
    }

    try
    {
        git::CommitMetadata metadata;

        metadata.name = _repository->getConfigValue("user.name");
        metadata.email = _repository->getConfigValue("user.email");

        metadata = CommitDialog::RunDialog(metadata);

        if (metadata.isValid())
        {
            _repository->createCommit(metadata);
        }

        analyseRemoteStatus(_repository);
    }
    catch (const git::GitException& ex)
    {
        wxutil::Messagebox::ShowError(ex.what());
    }

    {
        std::lock_guard<std::mutex> guard(_taskLock);
        _taskInProgress = false;
    }
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
    try
    {
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
