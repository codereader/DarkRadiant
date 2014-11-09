#include "GlobalKeyEventFilter.h"

#include <wx/event.h>
#include <wx/window.h>
#include <wx/textctrl.h>
#include <wx/stc/stc.h>
#include "wxutil/TreeView.h"

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
        wxObject* eventObject = keyEvent.GetEventObject();

        // Don't eat key events of text controls
        if (wxDynamicCast(eventObject, wxTextCtrl) || wxDynamicCast(eventObject, wxStyledTextCtrl))
        {
            return Event_Skip;
        }

        // Check if the event object can handle the event
        wxWindow* window = dynamic_cast<wxWindow*>(keyEvent.GetEventObject());

        if (!window) return Event_Skip;

        if (window->GetEventHandler()->ProcessEvent(keyEvent))
        {
            // The control handled this event, so don't check for accelerators
            return Event_Skip;
        }

        // Special handling for our treeviews with type ahead search

        // Treeviews are special, the actual wxWindows receiving/generating the event are the
        // privately implemented wxDataViewMainWindows, so let's attempt identifying that case
        // Check for keys with modifers, these are not handled by the treeview search
        if (!keyEvent.HasAnyModifiers() && 
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
                    return Event_Skip;
                }
            }
            else // non-escape key, no modifiers, don't process it
            {
                return Event_Skip;
            }
        }

        // The event stays unhandled by the control that it's triggered on
        // check if an accelerator matches this event
        // Try to find a matching accelerator
        AcceleratorList accelList = _eventManager.findAccelerator(keyEvent);

        if (!accelList.empty())
        {
            // Release any modifiers
            _eventManager.clearModifierState();

            // Pass the execute() call to all found accelerators
            for (AcceleratorList::iterator i = accelList.begin(); i != accelList.end(); ++i)
            {
                if (eventType == wxEVT_KEY_DOWN)
                {
                    i->keyDown();
                }
                else
                {
                    i->keyUp();
                }
            }

            return Event_Processed;
        }

        _eventManager.updateKeyState(keyEvent, eventType == wxEVT_KEY_DOWN);
    }

    // Continue processing the event normally as well.
    return Event_Skip;
}

}
