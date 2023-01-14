#include "AuiManager.h"

#include "AuiFloatingFrame.h"
#include "AuiLayout.h"

namespace ui
{

AuiManager::AuiManager(AuiLayout* layout) :
    wxAuiManager(nullptr, wxAUI_MGR_ALLOW_FLOATING | wxAUI_MGR_VENETIAN_BLINDS_HINT | wxAUI_MGR_LIVE_RESIZE),
    _layout(layout),
    _notebook(nullptr)
{}

void AuiManager::SetPropertyNotebook(PropertyNotebook* notebook)
{
    _notebook = notebook;
}

wxAuiFloatingFrame* AuiManager::CreateFloatingFrame(wxWindow* parent, const wxAuiPaneInfo& p)
{
    return new AuiFloatingFrame(parent, this, _notebook, p);
}

void AuiManager::DockPanelToNotebook(AuiFloatingFrame* frame)
{
    // Focus the notebook parent to avoid flickering
    wxGetTopLevelParent(_notebook)->SetFocus();

    _layout->convertFloatingPaneToPropertyTab(frame);
    _notebook->hideDropHint();
}

bool AuiManager::MouseCursorIsHoveringNotebook()
{
    auto point = wxGetMousePosition();
    return _notebook && _notebook->IsShownOnScreen() && _notebook->GetScreenRect().Contains(point);
}

bool AuiManager::CanDockPanel(const wxAuiPaneInfo& p)
{
    if (_notebook && MouseCursorIsHoveringNotebook())
    {
        // Block, if the mouse cursor is right on the tab control of the notebook
        HideHint();
        return false;
    }

    return wxAuiManager::CanDockPanel(p);
}

}
