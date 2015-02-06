#pragma once

#include <wx/event.h>
#include <wx/eventfilter.h>
#include <memory>

namespace ui
{

class EventManager;

/**
* Global filter owned by the EventManager module. Scans every
* key event in the application and invokes matching accelerators,
* unless the control in focus prevents this (like an active wxTextCtrl
* or wxutil::TreeView).
*/
class GlobalKeyEventFilter :
    public wxEventFilter
{
private:
    EventManager& _eventManager;

public:
    GlobalKeyEventFilter(EventManager& eventManager);
    virtual ~GlobalKeyEventFilter();

    virtual int FilterEvent(wxEvent& event);

private:
    // Returns true if this event applies for accelerator
    bool shouldConsiderEvent(wxKeyEvent& keyEvent);

    // Attempts to fire an accelerator for this event. Returns false if no accelerator matched.
    bool handleAccelerator(wxKeyEvent& keyEvent);
};
typedef std::shared_ptr<GlobalKeyEventFilter> GlobalKeyEventFilterPtr;

}
