#include "VcsStatus.h"

#include "i18n.h"
#include "iuserinterface.h"
#include "imainframe.h"

#include <wx/sizer.h>
#include <sigc++/functors/mem_fun.h>

#include "registry/registry.h"
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

    _text = new wxStaticText(this, wxID_ANY, "Statüs");
    GetSizer()->Add(_text);

    Bind(wxEVT_TIMER, &VcsStatus::onIntervalReached, this);

    GlobalRegistry().signalForKey(RKEY_AUTO_FETCH_ENABLED).connect(
        sigc::mem_fun(this, &VcsStatus::restartTimer)
    );
    GlobalRegistry().signalForKey(RKEY_AUTO_FETCH_INTERVAL).connect(
        sigc::mem_fun(this, &VcsStatus::restartTimer)
    );
}

VcsStatus::~VcsStatus()
{
    if (_fetchTask.valid())
    {
        _fetchTask.get(); // Wait for the thread to complete
    }
}

void VcsStatus::setRepository(const std::shared_ptr<git::Repository>& repository)
{
    _repository = repository;

    if (!_repository)
    {
        _timer.Stop();
        return;
    }

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

void VcsStatus::onIntervalReached(wxTimerEvent& ev)
{
    std::lock_guard<std::mutex> guard(_fetchLock);

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

void VcsStatus::performFetch(std::shared_ptr<git::Repository> repository)
{
    GlobalUserInterface().dispatch([this]() { _text->SetLabel(_("Fetching...")); });

    repository->fetchFromTrackedRemote();

    std::lock_guard<std::mutex> guard(_fetchLock);
    _fetchInProgress = false;

    auto status = repository->isUpToDateWithRemote() ? _("Up to date") : _("Updates available");
    GlobalUserInterface().dispatch([this, status]() { _text->SetLabel(status); });
}

}

}
