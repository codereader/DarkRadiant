#pragma once

#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <mutex>
#include <future>
#include <sigc++/trackable.h>

#include "imap.h"
#include "../Repository.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/menu/PopupMenu.h"

namespace vcs
{

namespace ui
{

class VcsStatus final :
    public wxEvtHandler,
    public wxutil::XmlResourceBasedWidget,
    public sigc::trackable
{
private:
    wxPanel* _panel;

    wxTimer _timer;
    std::mutex _taskLock;
    bool _fetchInProgress;
    std::future<void> _fetchTask;
    std::future<void> _mapFileTask;

    std::shared_ptr<git::Repository> _repository;

    wxStaticText* _remoteStatus;
    wxStaticText* _mapStatus;

    struct RemoteStatus
    {
        std::size_t localAheadCount;
        std::size_t remoteAheadCount;
        std::string label;
    };

    std::unique_ptr<wxutil::PopupMenu> _popupMenu;

public:
    static constexpr const char* const Name = "VcsStatusBarWidget";

    VcsStatus(wxWindow* parent);
    ~VcsStatus();

    wxWindow* getWidget();

    void setRepository(const std::shared_ptr<git::Repository>& repository);

private:
    void startFetchTask();
    void restartTimer();
    void onIntervalReached(wxTimerEvent& ev);
    void onIdle(wxIdleEvent& ev);
    void performFetch(std::shared_ptr<git::Repository> repository);
    void performMapFileStatusCheck(std::shared_ptr<git::Repository> repository);
    void updateMapFileStatus();
    void onMapEvent(IMap::MapEvent ev);
    void setMapFileStatus(const std::string& status);
    void setRemoteStatus(const RemoteStatus& status);

    RemoteStatus analyseRemoteStatus(const std::shared_ptr<git::Repository>& repository);
    std::string getRepositoryRelativePath(const std::string& path, const std::shared_ptr<git::Repository>& repository);
};

}

}
