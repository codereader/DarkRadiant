#pragma once

#include <memory>
#include <wx/eventfilter.h>
#include "../GLWidget.h"
#include "util/ScopedBoolLock.h"

namespace wxutil
{

/**
 * greebo: Propagate the mousewheel event to the GL window below the cursor
 * not the one that might happen to have focus in MS Windows
 */
class ScrollEventPropagationFilter final : 
	public wxEventFilter
{
private:
	bool _filterDisabled;
public:
	ScrollEventPropagationFilter() :
		_filterDisabled(false)
	{
		wxEvtHandler::AddFilter(this);
	}

	~ScrollEventPropagationFilter()
	{
		wxEvtHandler::RemoveFilter(this);
	}

	int FilterEvent(wxEvent& ev) override
	{
		if (_filterDisabled || ev.GetEventType() != wxEVT_MOUSEWHEEL)
		{
			return Event_Skip;
		}

		wxPoint mousePos = wxGetMousePosition();
		wxWindow* windowAtPoint = wxFindWindowAtPoint(mousePos);

        if (windowAtPoint)
        {
            // Special case for wxDataViewCtrls. wxDataViewMainWindow is the one that is returned by
            // the wxFindWindowAtPoint method, but it doesn't process the mouse wheel events, it's
            // its parent wxDataViewCtrl taking care of that - so with this special knowledge...
            if (wxString(windowAtPoint->GetClassInfo()->GetClassName()) == "wxDataViewMainWindow")
            {
                // ...redirect the mouse wheel event to the wxDataViewCtrl
                windowAtPoint = windowAtPoint->GetParent();
            }

            if (windowAtPoint)
            {
				// Disable this filter before passing it to the target window
				// it's possible that the wxEvtHandler of this window passes the event back to this filter
				util::ScopedBoolLock lock(_filterDisabled);
                return windowAtPoint->GetEventHandler()->ProcessEvent(ev) ? Event_Processed : Event_Skip;
            }
        }

		return Event_Skip;
	}
};
typedef std::shared_ptr<ScrollEventPropagationFilter> ScrollEventPropagationFilterPtr;

} // namespace
