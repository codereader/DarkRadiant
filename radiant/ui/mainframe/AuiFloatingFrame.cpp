#include "AuiFloatingFrame.h"

#include "AuiManager.h"
#include "PropertyNotebook.h"

namespace ui
{

AuiFloatingFrame::AuiFloatingFrame(wxWindow* parent, AuiManager* ownerMgr, PropertyNotebook* notebook, const wxAuiPaneInfo& pane) :
    wxAuiFloatingFrame(parent, ownerMgr, pane),
    _owner(ownerMgr),
    _notebook(notebook),
    _isMoving(false)
{
    Bind(wxEVT_MOVING, &AuiFloatingFrame::onMove, this);
    Bind(wxEVT_MOVE, &AuiFloatingFrame::onMove, this);
}

void AuiFloatingFrame::onIdle()
{
    if (!_isMoving) return;

    if (wxGetMouseState().LeftIsDown())
    {
        // Keep monitoring
        requestIdleCallback();
        return;
    }

    // Mouse button released
    _isMoving = false;
    _notebook->hideDropHint();

    if (_owner->MouseCursorIsHoveringNotebook())
    {
        _owner->DockPanelToNotebook(this);
    }
}

void AuiFloatingFrame::onMove(wxMoveEvent& ev)
{
    // Ignore dragging of the PropertyPanel itself
    // Only react to left-mouse drags
    if (!wxGetMouseState().LeftIsDown() || FindWindow(_notebook->GetId()))
    {
        ev.Skip();
        return;
    }

    _isMoving = true;
    requestIdleCallback();

    // Check if the mouse cursor is moved to the vicinity of a property notebook
    if (_owner->MouseCursorIsHoveringNotebook())
    {
        // Block this event, don't let the regular wxAuiFloatingFrame process it
        _owner->HideHint();
        ev.StopPropagation();

        _notebook->showDropHint(_notebook->GetScreenRect());
    }
    else
    {
        _notebook->hideDropHint();
        ev.Skip();
    }
}

}