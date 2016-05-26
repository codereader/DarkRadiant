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
public:
     // Result type to be returned by the callback to indicate what happened
    enum class Result
    {
        KeyProcessed,   // event has been processed, don't propagate further
        KeyIgnored,     // event can be propagated as usual
    };

    typedef std::function<Result()> Callback;

private:
    wxKeyCode _keyCodeToCapture;

    Callback _callback;

public:
    // Construct the filter with a keycode to observer and a callback
    // function that is invoked when the keycode occurs
    KeyEventFilter(wxKeyCode keyCodeToCapture, const Callback& callback) :
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
            Result result = Result::KeyProcessed;

            if (_callback)
            {
                result = _callback();
            }

            // Stop propagation if the key was processed
            return result == Result::KeyProcessed ? Event_Processed : Event_Skip;
        }

        // Continue processing the event normally if it doesn't match our signature
        return Event_Skip;
    }
};
typedef std::shared_ptr<KeyEventFilter> KeyEventFilterPtr;

}
