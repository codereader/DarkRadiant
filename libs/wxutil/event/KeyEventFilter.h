#pragma once

#include <wx/event.h>
#include <wx/eventfilter.h>
#include <memory>
#include <functional>

namespace wxutil
{

/**
 * A KeyEventFilter will register itself with wxWidgets on construction
 * and will invoke a given callback as soon as a defined keycode is 
 * encountered during keydown events.
 */
class KeyEventFilter :
    public wxEventFilter
{
private:
    wxKeyCode _keyCodeToCapture;

    std::function<void()> _callback;

public:
    // Construct the filter with a keycode to observer and a callback
    // function that is invoked when the keycode occurs
    KeyEventFilter(wxKeyCode keyCodeToCapture, const std::function<void()>& callback) :
        _keyCodeToCapture(keyCodeToCapture),
        _callback(callback)
    {
        wxEvtHandler::AddFilter(this);
    }

    virtual ~KeyEventFilter()
    {
        wxEvtHandler::RemoveFilter(this);
    }

    virtual int FilterEvent(wxEvent& event)
    {
        const wxEventType t = event.GetEventType();

        if (t == wxEVT_KEY_DOWN && 
            static_cast<wxKeyEvent&>(event).GetKeyCode() == _keyCodeToCapture)
        {
            if (_callback)
            {
                _callback();
            }

            // Stop propagation
            return Event_Processed;
        }

        // Continue processing the event normally as well.
        return Event_Skip;
    }
};
typedef std::shared_ptr<KeyEventFilter> KeyEventFilterPtr;

}
