#pragma once

#include "i18n.h"
#include "iuserinterface.h"
#include "imainframe.h"
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <mutex>
#include <future>
#include "../Repository.h"

namespace vcs
{

namespace ui
{

class VcsStatus final :
    public wxPanel
{
private:
    wxTimer _timer;
    std::mutex _fetchLock;
    bool _fetchInProgress;
    std::future<void> _fetchTask;

    std::shared_ptr<git::Repository> _repository;

    wxStaticText* _text;

public:
    static constexpr const char* const Name = "VcsStatusBarWidget";

    VcsStatus(wxWindow* parent) :
        wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER | wxWANTS_CHARS, "VcsStatusBarPanel"),
        _timer(this),
        _fetchInProgress(false)
    {
        SetSizer(new wxBoxSizer(wxHORIZONTAL));

        _text = new wxStaticText(this, wxID_ANY, "Statüs");
        GetSizer()->Add(_text);

        Bind(wxEVT_TIMER, &VcsStatus::onIntervalReached, this);
    }

    ~VcsStatus()
    {
        if (_fetchTask.valid())
        {
            _fetchTask.get(); // Wait for the thread to complete
        }
    }

    void setRepository(const std::shared_ptr<git::Repository>& repository)
    {
        _repository = repository;

        if (!_repository)
        {
            _timer.Stop();
            return;
        }

        _timer.Start(1000 * 5 * 60); // 5 mins
    }

private:
    void onIntervalReached(wxTimerEvent& ev)
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

    void performFetch(std::shared_ptr<git::Repository> repository)
    {
        GlobalUserInterface().dispatch([this]() { _text->SetLabel(_("Fetching...")); });

        repository->fetchFromTrackedRemote();

        std::lock_guard<std::mutex> guard(_fetchLock);
        _fetchInProgress = false;

        auto status = repository->isUpToDateWithRemote() ? _("Up to date") : _("Updates available");
        GlobalUserInterface().dispatch([&]() { _text->SetLabel(status); });
    }
};

}

}
