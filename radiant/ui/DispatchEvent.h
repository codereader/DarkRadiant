#pragma once

#include <wx/event.h>

namespace ui
{

/**
 * Event encapsulating an action that should be executed
 * on the primary (UI) thread.
 */
class DispatchEvent : 
	public wxEvent 
{
private:
    std::function<void()> _action;

public:
    DispatchEvent(wxEventType eventType, int winid, const std::function<void()>& action);

    DispatchEvent(const DispatchEvent& other) = default;

    const std::function<void()>& GetAction() const;

    // implement the base class pure virtual
    virtual wxEvent* Clone() const;
};

#define DispatchEventHandler(func) (&func)

wxDECLARE_EVENT(DISPATCH_EVENT, DispatchEvent);

}
