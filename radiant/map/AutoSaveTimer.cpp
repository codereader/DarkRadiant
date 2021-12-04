#include "AutoSaveTimer.h"

#include "ipreferencesystem.h"
#include "iautosaver.h"
#include "registry/registry.h"
#include "i18n.h"

namespace map
{

namespace
{
    constexpr const char* const RKEY_AUTOSAVE_INTERVAL = "user/ui/map/autoSaveInterval";
    constexpr const char* const RKEY_AUTOSAVE_ENABLED = "user/ui/map/autoSaveEnabled";
}

AutoSaveTimer::AutoSaveTimer() :
    _enabled(false),
    _interval(5 * 60)
{}

AutoSaveTimer::~AutoSaveTimer()
{
    _enabled = false;
    stopTimer();

    // Destroy the timer
    _timer.reset();
}

void AutoSaveTimer::initialise()
{
    // Add a page to the given group
    IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Autosave"));

    // Add the checkboxes and connect them with the registry key
    page.appendCheckBox(_("Enable Autosave"), RKEY_AUTOSAVE_ENABLED);
    page.appendSlider(_("Autosave Interval (in minutes)"), RKEY_AUTOSAVE_INTERVAL, 1, 61, 1, 1);

    _timer.reset(new wxTimer(this));

    Bind(wxEVT_TIMER, &AutoSaveTimer::onIntervalReached, this);

    GlobalRegistry().signalForKey(RKEY_AUTOSAVE_INTERVAL).connect(
        sigc::mem_fun(this, &AutoSaveTimer::registryKeyChanged)
    );
    GlobalRegistry().signalForKey(RKEY_AUTOSAVE_ENABLED).connect(
        sigc::mem_fun(this, &AutoSaveTimer::registryKeyChanged)
    );

    // Refresh all values from the registry right now (this might also start the timer)
    registryKeyChanged();
}

void AutoSaveTimer::registryKeyChanged()
{
    // Stop the current timer
    stopTimer();

    _enabled = registry::getValue<bool>(RKEY_AUTOSAVE_ENABLED);
    _interval = registry::getValue<int>(RKEY_AUTOSAVE_INTERVAL) * 60;

    // Start the timer with the new interval
    if (_enabled)
    {
        startTimer();
    }
}

void AutoSaveTimer::startTimer()
{
    if (!_timer) return;

    _timer->Start(static_cast<int>(_interval * 1000));
}

void AutoSaveTimer::stopTimer()
{
    if (!_timer) return;

    _timer->Stop();
}

void AutoSaveTimer::onIntervalReached(wxTimerEvent& ev)
{
    if (_enabled && GlobalAutoSaver().runAutosaveCheck())
    {
        // Stop the timer before saving
        stopTimer();

        GlobalAutoSaver().performAutosave();

        // Re-start the timer after saving has finished
        startTimer();
    }
}

}
