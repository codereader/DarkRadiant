#include "GlobalKeyEventFilter.h"

#include "imousetoolmanager.h"

#include <wx/event.h>
#include <wx/window.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <wx/stc/stc.h>
#include "wxutil/dataview/TreeView.h"
#include "wxutil/Modifier.h"
#include "wxutil/dialog/DialogBase.h"

#include "itextstream.h"
#include "EventManager.h"

namespace ui
{

    namespace
    {
        // Depending on our platform detecting the wxutil::TreeView from an eventobject
        // might be tricky. in wxMSW the events are fired from an internal wxDataViewMainWindow
        // and the parent is a TreeView, in wxGTK the events are fired on the TreeView* itself
        wxutil::TreeView* getTreeView(wxWindow* window)
        {
            wxutil::TreeView* view = dynamic_cast<wxutil::TreeView*>(window);
            
            return view != nullptr ? view : dynamic_cast<wxutil::TreeView*>(window->GetParent());
        }
    }

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

bool FilterInTextControls( wxKeyEvent& keyEvent ) {
    if ( keyEvent.ControlDown() ) {
        if ( keyEvent.GetKeyCode() > 32 && keyEvent.GetKeyCode() < 127 ) {
            switch ( keyEvent.GetKeyCode() ) {
            case 'C':case 'V':case 'X':case 'Y':case 'Z':
                return false;
            default:
                return true;
            }
        }
    }
    // For tool windows we let the ESC key propagate, since it's used to de-select stuff.
    return keyEvent.GetKeyCode() == WXK_ESCAPE;
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
        return FilterInTextControls(keyEvent) ? EventShouldBeProcessed : EventShouldBeIgnored;
    }

    // Special handling for our treeviews with type ahead search

    // Treeviews are special, the actual wxWindows receiving/generating the event are the
    // privately implemented wxDataViewMainWindows, so let's attempt identifying that case
    // Check for keys with Alt or Ctrl modifers, these are not handled by the treeview search
    wxutil::TreeView* treeView = getTreeView(window);

    if (treeView != nullptr && !keyEvent.ControlDown() && !keyEvent.AltDown())
    {
        // We have a modifier-less key event in a wxutil::TreeView. It will be passed through
        // in the general case. The ESC key will be caught if the treeview is not search mode.
        if (keyEvent.GetKeyCode() == WXK_ESCAPE)
        {
            // The ESC key should only be passed to the dataview if the search popup is actually shown
            if (treeView->HasActiveSearchPopup())
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
#if 1
    return _eventManager.handleKeyEvent(keyEvent);
#else
    AcceleratorList accelList = _eventManager.findAccelerator(keyEvent);

    if (!accelList.empty())
    {
        // Pass the execute() call to all found accelerators
        for (const auto& accelerator : accelList)
        {
            if (keyEvent.GetEventType() == wxEVT_KEY_DOWN)
            {
                accelerator->keyDown();
            }
            else
            {
                accelerator->keyUp();
            }
        }

        return true; // handled
    }

    return false; // no accelerator found
#endif
}

} // namespace
