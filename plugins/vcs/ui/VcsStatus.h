#pragma once

#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <mutex>
#include <future>
#include <sigc++/trackable.h>

#include "../Repository.h"

namespace vcs
{

namespace ui
{

class VcsStatus final :
    public wxPanel,
    public sigc::trackable
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

    VcsStatus(wxWindow* parent);
    ~VcsStatus();

    void setRepository(const std::shared_ptr<git::Repository>& repository);

private:
    void restartTimer();
    void onIntervalReached(wxTimerEvent& ev);
    void performFetch(std::shared_ptr<git::Repository> repository);
};

}

}
