#pragma once

#include <memory>
#include <wx/eventfilter.h>
#include "../GLWidget.h"

namespace wxutil
{

/**
 * greebo: Propagate the mousewheel event to the GL window below the cursor
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
            // Special case for wxDataViewCtrls. wxDataViewMainWindow is the one that is returned by
            // the wxFindWindowAtPointer method, but it doesn't process the mouse wheel events, it's
            // its parent wxDataViewCtrl taking care of that - so with this special knowledge...
            if (wxString(windowAtPoint->GetClassInfo()->GetClassName()) == "wxDataViewMainWindow")
            {
                // ...redirect the mouse wheel event to the wxDataViewCtrl
                windowAtPoint = windowAtPoint->GetParent();
            }

            if (windowAtPoint)
            {
                return windowAtPoint->GetEventHandler()->ProcessEvent(ev) ? Event_Processed : Event_Skip;
            }
        }

		return Event_Skip;
	}
};
typedef std::shared_ptr<ScrollEventPropagationFilter> ScrollEventPropagationFilterPtr;

} // namespace
