#pragma once

#include <wx/aui/aui.h>

namespace ui
{

class PropertyNotebook;
class AuiLayout;
class AuiFloatingFrame;

/**
 * Specialised Aui Manager implementation which allows dragging
 * floating panes into a property notebook for docking.
 */
class AuiManager :
    public wxAuiManager
{
private:
    AuiLayout* _layout;
    PropertyNotebook* _notebook;

public:
    AuiManager(AuiLayout* layout);

    void SetPropertyNotebook(PropertyNotebook* notebook);
    void DockPanelToNotebook(AuiFloatingFrame* frame);

    bool CanDockPanel(const wxAuiPaneInfo& p) override;
    wxAuiFloatingFrame* CreateFloatingFrame(wxWindow* parent, const wxAuiPaneInfo& p) override;

    bool MouseCursorIsHoveringNotebook();
};

}
