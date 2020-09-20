#include "DispatchEvent.h"

namespace ui
{

DispatchEvent::DispatchEvent(wxEventType eventType, int winid, const std::function<void()>& action) :
    wxEvent(winid, eventType),
    _action(action)
{}

const std::function<void()>& DispatchEvent::GetAction() const
{
    return _action;
}

// implement the base class pure virtual
wxEvent* DispatchEvent::Clone() const
{
    return new DispatchEvent(*this);
}

wxDEFINE_EVENT(DISPATCH_EVENT, DispatchEvent);

}
