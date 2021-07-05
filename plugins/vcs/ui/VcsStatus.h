#pragma once

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

class VcsStatus :
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

    void setRepository(const std::shared_ptr<git::Repository>& repository)
    {
        _repository = repository;

        if (!_repository)
        {
            _timer.Stop();
            return;
        }

        _timer.Start(1000 * 30); // 5 mins
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

        _text->SetLabel("Fetching...");
    }

    void performFetch(std::shared_ptr<git::Repository> repository)
    {
        rMessage() << "Async fetch from remote..." << std::endl;
        repository->fetchFromTrackedRemote();

        std::lock_guard<std::mutex> guard(_fetchLock);
        _fetchInProgress = false;
    }
};

}

}
