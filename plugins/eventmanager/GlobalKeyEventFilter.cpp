#include "GlobalKeyEventFilter.h"

#include "imousetoolmanager.h"

#include <wx/event.h>
#include <wx/window.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <wx/stc/stc.h>
#include "wxutil/TreeView.h"
#include "wxutil/Modifier.h"
#include "wxutil/dialog/DialogBase.h"

#include "itextstream.h"
#include "EventManager.h"

namespace ui
{

GlobalKeyEventFilter::GlobalKeyEventFilter(EventManager& eventManager) :
    _eventManager(eventManager)
{
    wxEvtHandler::AddFilter(this);
}

GlobalKeyEventFilter::~GlobalKeyEventFilter()
{
    wxEvtHandler::RemoveFilter(this);
}

int GlobalKeyEventFilter::FilterEvent(wxEvent& event)
{
    const wxEventType eventType = event.GetEventType();

    if (eventType == wxEVT_KEY_DOWN || eventType == wxEVT_KEY_UP)
    {
        wxKeyEvent& keyEvent = static_cast<wxKeyEvent&>(event);
        
		switch (checkEvent(keyEvent))
		{
		case EventShouldBeIgnored:
			return Event_Skip;

		case EventAlreadyProcessed:
			return Event_Processed;

		case EventShouldBeProcessed:
			{
				// Attempt to find an accelerator
				bool acceleratorFound = handleAccelerator(keyEvent);

				// Update the status bar in any case
				GlobalMouseToolManager().updateStatusbar(wxutil::Modifier::GetStateForKeyEvent(keyEvent));

				return acceleratorFound ? Event_Processed : Event_Skip;
			}
		};
    }

    // Continue processing the event normally for non-key events.
    return Event_Skip;
}

GlobalKeyEventFilter::EventCheckResult GlobalKeyEventFilter::checkEvent(wxKeyEvent& keyEvent)
{
    // Check if the event object can handle the event
    wxWindow* window = dynamic_cast<wxWindow*>(keyEvent.GetEventObject());

    if (!window) return EventShouldBeIgnored;

    if (window->GetEventHandler()->ProcessEvent(keyEvent))
    {
        // The control handled this event, so don't check for accelerators
        return EventAlreadyProcessed;
    }

    // Don't catch key events when a blocking dialog window is in focus
    wxWindow* topLevelParent = wxGetTopLevelParent(window);

    if (dynamic_cast<wxutil::DialogBase*>(topLevelParent))
    {
        return EventShouldBeIgnored;
    }

    wxObject* eventObject = keyEvent.GetEventObject();

    // Don't eat key events of text controls
    if (wxDynamicCast(eventObject, wxTextCtrl) || wxDynamicCast(eventObject, wxStyledTextCtrl) ||
        wxDynamicCast(eventObject, wxComboBox) || wxDynamicCast(eventObject, wxSpinCtrl) ||
        wxDynamicCast(eventObject, wxSpinCtrlDouble))
    {
        // For tool windows we let the ESC key propagate, since it's used to
        // de-select stuff.
        return keyEvent.GetKeyCode() == WXK_ESCAPE ? EventShouldBeProcessed : EventShouldBeIgnored;
    }

    // Special handling for our treeviews with type ahead search

    // Treeviews are special, the actual wxWindows receiving/generating the event are the
    // privately implemented wxDataViewMainWindows, so let's attempt identifying that case
    // Check for keys with Alt or Ctrl modifers, these are not handled by the treeview search
    if (!keyEvent.ControlDown() && !keyEvent.AltDown() &&
        wxString(eventObject->GetClassInfo()->GetClassName()) == "wxDataViewMainWindow")
    {
        // We have a modifier-less key event in a wxutil::TreeView. It will be passed through
        // in the general case. The ESC key will be caught if the treeview is not search mode.

        if (keyEvent.GetKeyCode() == WXK_ESCAPE)
        {
            // The ESC key should only be passed to the dataview if the search popup is actually shown
            wxutil::TreeView* treeView = dynamic_cast<wxutil::TreeView*>(window->GetParent());

            if (treeView && treeView->HasActiveSearchPopup())
            {
                return EventShouldBeIgnored;
            }
        }
        else // non-escape key, no modifiers, don't process it
        {
            return EventShouldBeIgnored;
        }
    }

    return EventShouldBeProcessed;
}

bool GlobalKeyEventFilter::handleAccelerator(wxKeyEvent& keyEvent)
{
    // The event stays unhandled by the control that it's triggered on
    // check if an accelerator matches this event
    // Try to find a matching accelerator
    AcceleratorList accelList = _eventManager.findAccelerator(keyEvent);

    if (!accelList.empty())
    {
        // Pass the execute() call to all found accelerators
        for (AcceleratorList::iterator i = accelList.begin(); i != accelList.end(); ++i)
        {
            if (keyEvent.GetEventType() == wxEVT_KEY_DOWN)
            {
                i->keyDown();
            }
            else
            {
                i->keyUp();
            }
        }

        return true; // handled
    }

    return false; // no accelerator found
}

} // namespace
