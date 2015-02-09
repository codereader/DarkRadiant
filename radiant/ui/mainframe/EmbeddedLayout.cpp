#include "EmbeddedLayout.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include <wx/sizer.h>
#include <wx/splitter.h>
#include <functional>

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

namespace ui
{

    namespace
    {
        const std::string RKEY_EMBEDDED_ROOT = "user/ui/mainFrame/embedded";
        const std::string RKEY_EMBEDDED_TEMP_ROOT = RKEY_EMBEDDED_ROOT + "/temp";
    }

std::string EmbeddedLayout::getName() {
    return EMBEDDED_LAYOUT_NAME;
}

void EmbeddedLayout::activate()
{
    wxFrame* topLevelParent = GlobalMainFrame().getWxTopLevelWindow();

    // Splitters
    _horizPane = new wxSplitterWindow(topLevelParent, wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "EmbeddedHorizPane");

    _horizPane->SetMinimumPaneSize(1); // disallow unsplitting
    _horizPane->SetSashGravity(0.5);
    _horizPane->SetSashPosition(400);

    GlobalMainFrame().getWxMainContainer()->Add(_horizPane, 1, wxEXPAND);

    // Allocate a new OrthoView and set its ViewType to XY
    XYWndPtr xywnd = GlobalXYWnd().createEmbeddedOrthoView(XY, _horizPane);

    // CamGroup Pane
    _groupCamPane = new wxSplitterWindow(_horizPane, wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "EmbeddedVertPane");

    _groupCamPane->SetSashGravity(0.5);
    _groupCamPane->SetSashPosition(300);
    _groupCamPane->SetMinimumPaneSize(1); // disallow unsplitting

    // Create a new camera window and parent it
    _camWnd = GlobalCamera().createCamWnd(_groupCamPane);

    wxPanel* notebookPanel = new wxPanel(_groupCamPane, wxID_ANY);
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

        GlobalGroupDialog().addPage(page);
    }

    _groupCamPane->SplitHorizontally(_camWnd->getMainWidget(), notebookPanel);

    // Add the camGroup pane to the left and the GL widget to the right
    _horizPane->SplitVertically(_groupCamPane, xywnd->getGLWidget());

    // Connect the pane position trackers
    _posHPane.connect(_horizPane);
    _posGroupCamPane.connect(_groupCamPane);

    // Attempt to restore this layout's state
    restoreStateFromPath(RKEY_EMBEDDED_ROOT);

    topLevelParent->Layout();

    // Hide the camera toggle option for non-floating views
    GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", false);
    // Hide the console/texture browser toggles for non-floating/non-split views
    GlobalUIManager().getMenuManager().setVisibility("main/view/textureBrowser", false);
}

void EmbeddedLayout::deactivate()
{
    // Show the camera toggle option again
    GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", true);
    GlobalUIManager().getMenuManager().setVisibility("main/view/textureBrowser", true);

    // Remove all previously stored pane information
    GlobalRegistry().deleteXPath(RKEY_EMBEDDED_ROOT + "//pane");

    // Save pane info
    saveStateToPath(RKEY_EMBEDDED_ROOT);

    // Delete all active views
    GlobalXYWndManager().destroyViews();

    // Delete the CamWnd
    _camWnd = CamWndPtr();

    // Give the notebook back to the GroupDialog
    GlobalGroupDialog().reparentNotebookToSelf();

    // Hide the group dialog
    GlobalGroupDialog().hideDialogWindow();

    GlobalGroupDialog().removePage("textures"); // do this after destroyWindow()

    // Disconnect before destroying stuff
    _posHPane.disconnect();
    _posGroupCamPane.disconnect();

    wxFrame* topLevelParent = GlobalMainFrame().getWxTopLevelWindow();
    topLevelParent->RemoveChild(_horizPane);
    _horizPane->Destroy();

    // Those two have been deleted by the above, so NULL the references
    _horizPane = NULL;
    _groupCamPane = NULL;
}

void EmbeddedLayout::maximiseCameraSize()
{
    // Save the current state to the registry
    saveStateToPath(RKEY_EMBEDDED_TEMP_ROOT);

    // Maximise the camera, wxWidgets will clip the coordinates
    _horizPane->SetSashPosition(2000000);
    _groupCamPane->SetSashPosition(2000000);
}

void EmbeddedLayout::restorePanePositions()
{
    // Restore state
    restoreStateFromPath(RKEY_EMBEDDED_TEMP_ROOT);

    // Remove all previously stored pane information
    GlobalRegistry().deleteXPath(RKEY_EMBEDDED_TEMP_ROOT);
}

void EmbeddedLayout::restoreStateFromPath(const std::string& path)
{
    // Trigger a proper resize event before setting the sash position
    GlobalMainFrame().getWxTopLevelWindow()->SendSizeEvent();
    wxTheApp->Yield();

    // Now load the paned positions from the registry
    if (GlobalRegistry().keyExists(path + "/pane[@name='texcam']"))
    {
        _posGroupCamPane.loadFromPath(path + "/pane[@name='texcam']");
    }

    if (GlobalRegistry().keyExists(path + "/pane[@name='horizontal']"))
    {
        _posHPane.loadFromPath(path + "/pane[@name='horizontal']");
    }
}

void EmbeddedLayout::saveStateToPath(const std::string& path)
{
    GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
    _posHPane.saveToPath(path + "/pane[@name='horizontal']");

    GlobalRegistry().createKeyWithName(path, "pane", "texcam");
    _posGroupCamPane.saveToPath(path + "/pane[@name='texcam']");
}

void EmbeddedLayout::toggleFullscreenCameraView()
{
    if (GlobalRegistry().keyExists(RKEY_EMBEDDED_TEMP_ROOT))
    {
        restorePanePositions();
    }
    else
    {
        // No saved info found in registry, maximise cam
        maximiseCameraSize();
    }
}

// The creation function, needed by the mainframe layout manager
EmbeddedLayoutPtr EmbeddedLayout::CreateInstance()
{
    return EmbeddedLayoutPtr(new EmbeddedLayout);
}

} // namespace ui
