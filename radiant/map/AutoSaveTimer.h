#pragma once

#include <sigc++/trackable.h>
#include <wx/timer.h>
#include <wx/sharedptr.h>

namespace map
{

/**
 * Timer object which repeatedly triggers the automatic map save algorithms
 * in the configured intervals.
 */
class AutoSaveTimer final :
    public wxEvtHandler,
    public sigc::trackable
{
private:
    // TRUE, if autosaving is enabled
    bool _enabled;

    // The autosave interval stored in seconds
    unsigned long _interval;

    // The timer object that triggers the callback
    wxSharedPtr<wxTimer> _timer;

public:
    AutoSaveTimer();
    ~AutoSaveTimer();

    void initialise();

    // Starts/stops the autosave "countdown"
    void startTimer();
    void stopTimer();

private:
    void registryKeyChanged();
    void onIntervalReached(wxTimerEvent& ev);
};

}
