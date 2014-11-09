#pragma once

#include <wx/eventfilter.h>
#include <memory>

class EventManager;

namespace ui
{

/**
* TODO
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
};
typedef std::shared_ptr<GlobalKeyEventFilter> GlobalKeyEventFilterPtr;

}
