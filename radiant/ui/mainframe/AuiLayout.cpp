#include "AuiLayout.h"

#include "i18n.h"
#include "ui/imenumanager.h"
#include "ui/igroupdialog.h"
#include "ui/imainframe.h"
#include "ui/iuserinterface.h"

#include <wx/sizer.h>
#include <wx/aui/auibook.h>

#include "camera/CameraWndManager.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "wxutil/Bitmap.h"
#include "xyview/GlobalXYWnd.h"

namespace ui
{

namespace
{
    const std::string RKEY_ROOT = "user/ui/mainFrame/aui";

    // Minimum size of docked panels
    const wxSize MIN_SIZE(128, 128);

    // Return a pane info with default options
    wxAuiPaneInfo DEFAULT_PANE_INFO(const std::string& caption,
                                    const wxSize& minSize)
    {
        return wxAuiPaneInfo().Caption(caption).CloseButton(false).MaximizeButton()
                              .BestSize(minSize).MinSize(minSize).DestroyOnClose(true);
    }
}

AuiLayout::AuiLayout() :
    _auiMgr(nullptr, wxAUI_MGR_ALLOW_FLOATING | wxAUI_MGR_VENETIAN_BLINDS_HINT | wxAUI_MGR_LIVE_RESIZE),
    _propertyNotebook(nullptr)
{
    _auiMgr.Bind(wxEVT_AUI_PANE_CLOSE, &AuiLayout::onPaneClose, this);
}

void AuiLayout::addPane(const std::string& name, wxWindow* window, const wxAuiPaneInfo& info)
{
    // Give the pane a deterministic name so we can restore perspective
    wxAuiPaneInfo nameInfo = info;
    nameInfo.Name(std::to_string(_panes.size()));

    // Add and store the pane
    _auiMgr.AddPane(window, nameInfo);
    _panes.push_back({ name, window });
}

void AuiLayout::onPaneClose(wxAuiManagerEvent& ev)
{
    // This is a desperate work around to let undocked property windows
    // return to the property notebook when they're closed
    // I failed finding any other way to have floating windows dragged into
    // the notebook, or adding a custom pane button - I'm open to ideas
    auto closedPane = ev.GetPane();

    for (const auto& info : _panes)
    {
        if (info.control == closedPane->window)
        {
            _propertyNotebook->addControl(info.controlName);
            break;
        }
    }
}

std::string AuiLayout::getName()
{
    return AUI_LAYOUT_NAME;
}

PropertyNotebook* AuiLayout::getNotebook()
{
    return _propertyNotebook;
}

void AuiLayout::activate()
{
    auto topLevelParent = GlobalMainFrame().getWxTopLevelWindow();

    // AUI manager can't manage a Sizer, we need to create an actual wxWindow
    // container
    auto managedArea = new wxWindow(topLevelParent, wxID_ANY);
    _auiMgr.SetManagedWindow(managedArea);
    GlobalMainFrame().getWxMainContainer()->Add(managedArea, 1, wxEXPAND);

    _propertyNotebook = new PropertyNotebook(managedArea, *this);

    auto orthoViewControl = GlobalUserInterface().findControl(UserControl::OrthoView);
    auto cameraControl = GlobalUserInterface().findControl(UserControl::Camera);
    assert(cameraControl);
    assert(orthoViewControl);

    // Add the camera and notebook to the left, as with the Embedded layout, and
    // the 2D view on the right
    wxSize size = topLevelParent->GetSize();
    size.Scale(0.5, 1.0);
    addPane(cameraControl->getControlName(), cameraControl->createWidget(managedArea),
            DEFAULT_PANE_INFO(cameraControl->getDisplayName(), size).Left().Position(0));
    addPane("PropertiesPanel", _propertyNotebook,
            DEFAULT_PANE_INFO(_("Properties"), size).Left().Position(1));
    addPane(orthoViewControl->getControlName(), orthoViewControl->createWidget(managedArea),
            DEFAULT_PANE_INFO(orthoViewControl->getDisplayName(), size).CenterPane());
    _auiMgr.Update();

    // Hide the camera toggle option for non-floating views
    GlobalMenuManager().setVisibility("main/view/cameraview", false);
    // Hide the console/texture browser toggles for non-floating/non-split views
    GlobalMenuManager().setVisibility("main/view/textureBrowser", false);
}

void AuiLayout::deactivate()
{
    // Save all floating XYWnd states
    GlobalXYWnd().saveState();

    // Store perspective
    GlobalRegistry().set(RKEY_ROOT, _auiMgr.SavePerspective().ToStdString());

    // Show the camera toggle option again
    GlobalMenuManager().setVisibility("main/view/cameraview", true);
    GlobalMenuManager().setVisibility("main/view/textureBrowser", true);

    // Remove all previously stored pane information
    // GlobalRegistry().deleteXPath(RKEY_ROOT + "//pane");

    // Delete all active views
    GlobalXYWndManager().destroyViews();

    // Get a reference to the managed window, it might be cleared by UnInit()
    auto managedWindow = _auiMgr.GetManagedWindow();

    // Unregister the AuiMgr from the event handlers of the managed window
    // otherwise we run into crashes during shutdown (#5586)
    _auiMgr.UnInit();

    managedWindow->Destroy();
}

void AuiLayout::createFloatingControl(const std::string& controlName)
{
    auto control = GlobalUserInterface().findControl(controlName);

    if (!control)
    {
        rError() << "Cannot create floating control: " << controlName << std::endl;
        return;
    }

    auto managedWindow = _auiMgr.GetManagedWindow();

    auto pane = DEFAULT_PANE_INFO(control->getDisplayName(), wxSize(250, 450))
        .Float().CloseButton(true);

    if (!control->getIcon().empty())
    {
        pane.Icon(wxutil::GetLocalBitmap(control->getIcon()));
    }

    addPane(control->getControlName(), control->createWidget(managedWindow), pane);
    _auiMgr.Update();
}

void AuiLayout::restoreStateFromRegistry()
{
    // Nasty hack to get the panes sized properly. Since BestSize() is
    // completely ignored (at least on Linux), we have to add the panes with a
    // large *minimum* size and then reset this size after the initial addition.
    for (const auto& info : _panes)
    {
        _auiMgr.GetPane(info.control).MinSize(MIN_SIZE);
    }
    _auiMgr.Update();

    // If we have a stored perspective, load it
    std::string storedPersp = GlobalRegistry().get(RKEY_ROOT);
    if (!storedPersp.empty())
    {
        _auiMgr.LoadPerspective(storedPersp);
    }

    // Restore all floating XY views
    GlobalXYWnd().restoreState();
}

void AuiLayout::toggleFullscreenCameraView()
{
}

std::shared_ptr<AuiLayout> AuiLayout::CreateInstance()
{
    return std::make_shared<AuiLayout>();
}

} // namespace ui
