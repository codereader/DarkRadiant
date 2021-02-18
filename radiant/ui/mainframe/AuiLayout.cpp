#include "AuiLayout.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "imenumanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include <wx/sizer.h>
#include <wx/splitter.h>
#include <functional>

#include "camera/CameraWndManager.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

namespace ui
{

namespace
{
    const std::string RKEY_ROOT = "user/ui/mainFrame/aui";

    // Return a pane info with default options
    wxAuiPaneInfo DEFAULT_PANE_INFO(const std::string& caption)
    {
        return wxAuiPaneInfo().Caption(caption).Layer(1).CloseButton(false)
                              .MinSize(wxSize(128, 128));
    }
}

AuiLayout::AuiLayout()
: _auiMgr(nullptr, wxAUI_MGR_ALLOW_FLOATING | wxAUI_MGR_VENETIAN_BLINDS_HINT |
                   wxAUI_MGR_LIVE_RESIZE)
{
}

std::string AuiLayout::getName()
{
    return AUI_LAYOUT_NAME;
}

void AuiLayout::activate()
{
    wxFrame* topLevelParent = GlobalMainFrame().getWxTopLevelWindow();

    // AUI manager can't manage a Sizer, we need to create an actual wxWindow
    // container
    wxWindow* managedArea = new wxWindow(topLevelParent, wxID_ANY);
    _auiMgr.SetManagedWindow(managedArea);
    GlobalMainFrame().getWxMainContainer()->Add(managedArea, 1, wxEXPAND);

    // Allocate a new OrthoView and set its ViewType to XY
    XYWndPtr xywnd = GlobalXYWnd().createEmbeddedOrthoView(XY, managedArea);

    // Create a new camera window and parent it
    _camWnd = GlobalCamera().createCamWnd(managedArea);

    wxPanel* notebookPanel = new wxPanel(managedArea, wxID_ANY);
    notebookPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

    GlobalGroupDialog().reparentNotebook(notebookPanel);

    // Hide the floating window
    GlobalGroupDialog().hideDialogWindow();

    // Add a new texture browser to the group dialog pages
    wxWindow* textureBrowser = new TextureBrowser(notebookPanel);

    // Texture Page
    {
        IGroupDialog::PagePtr page(new IGroupDialog::Page);

        page->name = "textures";
        page->windowLabel = _("Texture Browser");
        page->page = textureBrowser;
        page->tabIcon = "icon_texture.png";
        page->tabLabel = _("Textures");
		page->position = IGroupDialog::Page::Position::TextureBrowser;

        GlobalGroupDialog().addPage(page);
    }

    // Add the camera and notebook to the left, as with the Embedded layout, and
    // the 2D view on the right
    _auiMgr.AddPane(_camWnd->getMainWidget(),
                    DEFAULT_PANE_INFO(_("Camera")).Left().Position(0));
    _auiMgr.AddPane(notebookPanel,
                    DEFAULT_PANE_INFO(_("Properties")).Left().Position(1));
    _auiMgr.AddPane(xywnd->getGLWidget(),
                    DEFAULT_PANE_INFO(_("2D View")).Right());
    _auiMgr.Update();

    // Hide the camera toggle option for non-floating views
    GlobalMenuManager().setVisibility("main/view/cameraview", false);
    // Hide the console/texture browser toggles for non-floating/non-split views
    GlobalMenuManager().setVisibility("main/view/textureBrowser", false);
}

void AuiLayout::deactivate()
{
    // Show the camera toggle option again
    GlobalMenuManager().setVisibility("main/view/cameraview", true);
    GlobalMenuManager().setVisibility("main/view/textureBrowser", true);

    // Remove all previously stored pane information
    // GlobalRegistry().deleteXPath(RKEY_ROOT + "//pane");

    // Delete all active views
    GlobalXYWndManager().destroyViews();

    // Delete the CamWnd
    _camWnd.reset();

    // Give the notebook back to the GroupDialog
    GlobalGroupDialog().reparentNotebookToSelf();

    // Hide the group dialog
    GlobalGroupDialog().hideDialogWindow();

    GlobalGroupDialog().removePage("textures"); // do this after destroyWindow()
}

void AuiLayout::restoreStateFromRegistry()
{
}

void AuiLayout::toggleFullscreenCameraView()
{
}

// The creation function, needed by the mainframe layout manager
AuiLayoutPtr AuiLayout::CreateInstance()
{
    return AuiLayoutPtr(new AuiLayout);
}

} // namespace ui
