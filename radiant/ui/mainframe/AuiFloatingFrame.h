#pragma once

#include <wx/aui/aui.h>

#include "wxutil/event/SingleIdleCallback.h"

class wxAuiPaneInfo;

namespace ui
{

class AuiManager;
class PropertyNotebook;

/**
 * Specialised frame implementation used for floating panels in
 * DarkRadiant's mainframe. It detects being dragged over the property notebook
 * and docks as a new tab when dropped there.
 */
class AuiFloatingFrame :
    public wxAuiFloatingFrame,
    public wxutil::SingleIdleCallback
{
private:
    AuiManager* _owner;
    PropertyNotebook* _notebook;
    bool _isMoving;

public:
    AuiFloatingFrame(wxWindow* parent, AuiManager* ownerMgr, PropertyNotebook* notebook, const wxAuiPaneInfo& pane);

protected:
    void onIdle() override;

private:
    void onMove(wxMoveEvent& ev);
};

}
