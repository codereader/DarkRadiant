#pragma once

#include <wx/popupwin.h>
#include <wx/app.h>
#include <wx/sizer.h>
#include "MultiMonitor.h"

namespace wxutil
{
/**
 * A popup window that is destroying itself as soon as it loses focus
 */
class TransientPopupWindow :
    public wxPopupTransientWindow,
    public wxEventFilter
{
public:
    TransientPopupWindow(wxWindow* parent) :
        wxPopupTransientWindow(parent, wxBORDER_DEFAULT)
    {
        SetSizer(new wxBoxSizer(wxVERTICAL));

        // Register as global filter to catch events
        wxEvtHandler::AddFilter(this);
    }

    ~TransientPopupWindow() override
    {
        wxEvtHandler::RemoveFilter(this);
    }

    void Dismiss() override
    {
        wxPopupTransientWindow::Dismiss();
        Destroy();
    }

    int FilterEvent(wxEvent& ev) override
    {
        // Close the popup on any ESC keypress
        if (ev.GetEventType() == wxEVT_KEY_DOWN && static_cast<wxKeyEvent&>(ev).GetKeyCode() == WXK_ESCAPE)
        {
            Dismiss();
            return Event_Processed;
        }
        
        if (ev.GetEventType() == wxEVT_LEFT_DOWN || ev.GetEventType() == wxEVT_RIGHT_DOWN ||
            ev.GetEventType() == wxEVT_MIDDLE_DOWN || ev.GetEventType() == wxEVT_AUX1_DOWN ||
            ev.GetEventType() == wxEVT_AUX2_DOWN)
        {
            auto& mouseEvent = static_cast<wxMouseEvent&>(ev);
            auto window = static_cast<wxWindow*>(ev.GetEventObject());

            // Convert to coordinates of this window
            auto position = this->ScreenToClient(window->ClientToScreen(mouseEvent.GetPosition()));

            // Close if the click is outside of the window, but prevent double deletions
            if (this->HitTest(position.x, position.y) == wxHT_WINDOW_OUTSIDE &&
                !wxTheApp->IsScheduledForDestruction(this))
            {
                Dismiss();
            }
        }

        return Event_Skip;
    }

    // Attempts to position the popup to the right or left of the given window
    // The vertical offset is added to the screen position of the given window
    void PositionNextTo(wxWindow* win, int verticalOffset, const wxSize& size)
    {
        auto rectScreen = MultiMonitor::getMonitorForWindow(win);

        SetSize(size);

        // Try right first
        auto rightTop = win->GetScreenRect().GetRightTop();
        rightTop.y += verticalOffset;

        if (rightTop.x + size.GetWidth() < rectScreen.GetX() + rectScreen.GetWidth())
        {
            SetPosition(rightTop);
            return;
        }

        // Failed, place to the left
        auto leftTop = win->GetScreenRect().GetLeftTop();
        leftTop.y += verticalOffset;

        if (leftTop.x - size.GetWidth() > rectScreen.GetX())
        {
            SetPosition(wxPoint(leftTop.x - size.GetWidth(), leftTop.y));
            return;
        }

        // Fall back to the wxwidgets default algorithm
        Position(win->GetScreenPosition(), size);
    }
};

}
