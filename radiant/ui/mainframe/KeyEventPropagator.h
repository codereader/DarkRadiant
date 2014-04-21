#pragma once

#include <wx/event.h>
#include <wx/panel.h>

namespace ui
{

/**
 * greebo: We're using a lot of panels which tend to draw keyboard focus to themselves.
 * As wxEVT_KEY_* events are not propagated by wxWidgets by default, we need
 * to force them to, if a wxPanel is in focus in the first place.
 */
class KeyEventPropagationFilter : 
	public wxEventFilter
{
public:
	KeyEventPropagationFilter()
	{
		wxEvtHandler::AddFilter(this);
	}

	virtual ~KeyEventPropagationFilter()
	{
		wxEvtHandler::RemoveFilter(this);
	}

	virtual int FilterEvent(wxEvent& event)
	{
		const wxEventType t = event.GetEventType();

		if ((t == wxEVT_KEY_DOWN || t == wxEVT_KEY_UP) && 
			dynamic_cast<wxPanel*>(event.GetEventObject()) != NULL)
		{
			event.ResumePropagation(wxINT32_MAX);
		}

		// Continue processing the event normally as well.
		return Event_Skip;
	}
};

} // namespace
