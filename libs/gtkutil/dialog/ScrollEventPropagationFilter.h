#pragma once

#include <memory>
#include <wx/eventfilter.h>

namespace wxutil
{

/**
 * greebo: Propagate the mousewheel event to the window below the cursor
 * not the one that might happen to have focus in MS Windows
 */
class ScrollEventPropagationFilter : 
	public wxEventFilter
{
public:
	ScrollEventPropagationFilter()
	{
		wxEvtHandler::AddFilter(this);
	}

	virtual ~ScrollEventPropagationFilter()
	{
		wxEvtHandler::RemoveFilter(this);
	}

	virtual int FilterEvent(wxEvent& ev)
	{
		if (ev.GetEventType() != wxEVT_MOUSEWHEEL)
		{
			return Event_Skip;
		}

		wxPoint mousePos = wxGetMousePosition();
		wxWindow* windowAtPoint = wxFindWindowAtPointer(mousePos);

		if (windowAtPoint) 
		{
			windowAtPoint->GetEventHandler()->AddPendingEvent(ev);
			return Event_Processed; // intercepted
		}

		// Continue processing the event normally as well.
		return Event_Skip;
	}
};
typedef std::shared_ptr<ScrollEventPropagationFilter> ScrollEventPropagationFilterPtr;

} // namespace
