#pragma once

#include <wx/popupwin.h>
#include <wx/sizer.h>
#include "MultiMonitor.h"

namespace wxutil
{
/**
 * A popup window that is destroying itself as soon as it loses focus
 */
class TransientPopupWindow :
    public wxPopupTransientWindow
{
public:
    TransientPopupWindow(wxWindow* parent) :
        wxPopupTransientWindow(parent, wxBORDER_DEFAULT)
    {
        SetSizer(new wxBoxSizer(wxVERTICAL));
    }

    void Dismiss() override
    {
        wxPopupTransientWindow::Dismiss();
        Destroy();
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
