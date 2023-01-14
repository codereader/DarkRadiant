#include "AuiLayout.h"

#include "i18n.h"
#include "ui/imenumanager.h"
#include "ui/imainframe.h"
#include "ui/iuserinterface.h"

#include <wx/sizer.h>
#include <wx/aui/auibook.h>

#include "camera/CameraWndManager.h"
#include "command/ExecutionFailure.h"
#include "wxutil/Bitmap.h"
#include "xyview/GlobalXYWnd.h"
#include "AuiFloatingFrame.h"

namespace ui
{

namespace
{
    const std::string RKEY_ROOT = "user/ui/mainFrame/aui/";
    const std::string RKEY_AUI_PERSPECTIVE = RKEY_ROOT + "perspective";
    const std::string RKEY_AUI_PANES = RKEY_ROOT + "panes";
    const std::string RKEY_AUI_FLOATING_PANE_LOCATIONS = RKEY_ROOT + "floatingPaneLocations";
    const std::string RKEY_AUI_DOCKED_PANE_LOCATIONS = RKEY_ROOT + "dockedPaneLocations";
    const std::string RKEY_AUI_LAYOUT_VERSION = RKEY_ROOT + "layoutVersion";
    const std::string PANE_NODE_NAME = "pane";
    const std::string PANE_NAME_ATTRIBUTE = "paneName";
    const std::string STATE_ATTRIBUTE = "state";
    const std::string CONTROL_NAME_ATTRIBUTE = "controlName";
    constexpr const char* const  PROPERTIES_PANEL = "PropertiesPanel";
    constexpr int AuiLayoutVersion = 1;

    // Minimum size of docked panels
    const wxSize MIN_SIZE(128, 128);

    // Return a pane info with default options
    wxAuiPaneInfo DEFAULT_PANE_INFO(const std::string& caption,
                                    const wxSize& minSize)
    {
        return wxAuiPaneInfo().Caption(caption).CloseButton(false).MaximizeButton()
                              .BestSize(minSize).MinSize(minSize).DestroyOnClose(true);
    }

    void setupFloatingPane(wxAuiPaneInfo& pane)
    {
        pane.Float().CloseButton(true).MinSize(128, 128);
    }

    inline bool isCenterPane(const wxAuiPaneInfo& pane)
    {
        return pane.dock_direction == wxAUI_DOCK_CENTER;
    }
}

AuiLayout::AuiLayout() :
    _auiMgr(this),
    _propertyNotebook(nullptr)
{
    _auiMgr.GetArtProvider()->SetMetric(wxAUI_DOCKART_GRADIENT_TYPE, wxAUI_GRADIENT_NONE);
    _auiMgr.Bind(wxEVT_AUI_PANE_CLOSE, &AuiLayout::onPaneClose, this);
}

bool AuiLayout::paneNameExists(const std::string& name) const
{
    for (const auto& info : _panes)
    {
        if (info.paneName == name)
        {
            return true;
        }
    }

    return false;
}

bool AuiLayout::controlExists(const std::string& controlName) const
{
    // Check the notebook first
    if (_propertyNotebook->controlExists(controlName))
    {
        return true;
    }

    for (const auto& info : _panes)
    {
        if (info.controlName == controlName)
        {
            return true;
        }
    }

    return false;
}

std::string AuiLayout::generateUniquePaneName(const std::string& controlName)
{
    auto paneName = controlName;
    auto index = 1;

    while (paneNameExists(paneName))
    {
        paneName = fmt::format("{0}{1}", controlName, ++index);
    }

    return paneName;
}

void AuiLayout::addPane(const std::string& controlName, wxWindow* window, const wxAuiPaneInfo& info)
{
    // Give the pane a unique name so we can restore perspectives
    addPane(controlName, generateUniquePaneName(controlName), window, info);
}

void AuiLayout::addPane(const std::string& controlName, const std::string& paneName, wxWindow* window, const wxAuiPaneInfo& info)
{
    wxAuiPaneInfo paneInfo = info;
    paneInfo.Name(paneName);

    // Add and store the pane
    _auiMgr.AddPane(window, paneInfo);
    // Remember this pane
    _panes.push_back({ paneName, controlName, window });
}

void AuiLayout::convertPaneToPropertyTab(const std::string& paneName)
{
    for (auto i = _panes.begin(); i != _panes.end(); ++i)
    {
        if (i->paneName != paneName) continue;

        // Close the pane if it's present
        if (auto paneInfo = _auiMgr.GetPane(i->paneName); paneInfo.IsOk())
        {
            _auiMgr.ClosePane(paneInfo);
        }

        _propertyNotebook->addControl(i->controlName);
        _panes.erase(i);
        break;
    }
}

void AuiLayout::convertFloatingPaneToPropertyTab(AuiFloatingFrame* floatingWindow)
{
    for (auto i = _panes.begin(); i != _panes.end(); ++i)
    {
        auto& paneInfo = _auiMgr.GetPane(i->paneName);

        if (paneInfo.IsOk() && paneInfo.IsFloating() && paneInfo.frame == floatingWindow)
        {
            // Close the floating pane
            convertPaneToPropertyTab(i->paneName);
            break;
        }
    }
}

void AuiLayout::handlePaneClosed(wxAuiPaneInfo& pane)
{
    // Store the position of this pane before closing
    savePaneLocation(pane);

    ensureControlIsInactive(pane.window);

    // If this is the (floating) property panel that is closing,
    // notify it to ensure the last active tab is becoming inactive
    if (pane.window == _propertyNotebook)
    {
        _propertyNotebook->onNotebookPaneClosed();
    }

    // Find and remove the pane info structure
    for (auto i = _panes.begin(); i != _panes.end(); ++i)
    {
        if (i->paneName != pane.name) continue;

        _panes.erase(i);
        break;
    }

    // Avoid losing focus of the main window
    wxGetTopLevelParent(_auiMgr.GetManagedWindow())->SetFocus();
}

void AuiLayout::onPaneClose(wxAuiManagerEvent& ev)
{
    // For floating panes, memorise the last size and position
    auto closedPane = ev.GetPane();

    handlePaneClosed(*closedPane);
}

void AuiLayout::saveStateToRegistry()
{
    registry::setValue(RKEY_AUI_LAYOUT_VERSION, AuiLayoutVersion);

    // Save the pane perspective
    registry::setValue(RKEY_AUI_PERSPECTIVE, _auiMgr.SavePerspective().ToStdString());

    // Save tracked panes, we need to create all named panes before we can load the perspective
    GlobalRegistry().deleteXPath(RKEY_AUI_PANES);

    auto panesKey = GlobalRegistry().createKey(RKEY_AUI_PANES);

    for (const auto& pane : _panes)
    {
        auto& paneInfo = _auiMgr.GetPane(pane.control);

        // Skip hidden panels
        if (!paneInfo.IsShown())
        {
            continue;
        }

        auto paneNode = panesKey.createChild(PANE_NODE_NAME);
        paneNode.setAttributeValue(CONTROL_NAME_ATTRIBUTE, pane.controlName);
        paneNode.setAttributeValue(PANE_NAME_ATTRIBUTE, pane.paneName);
    }

    // Persist the floating pane locations
    GlobalRegistry().deleteXPath(RKEY_AUI_FLOATING_PANE_LOCATIONS);
    auto floatingPanesKey = GlobalRegistry().createKey(RKEY_AUI_FLOATING_PANE_LOCATIONS);

    for (const auto& [name, state] : _floatingPaneLocations)
    {
        auto paneNode = floatingPanesKey.createChild(PANE_NODE_NAME);
        paneNode.setAttributeValue(PANE_NAME_ATTRIBUTE, name);
        paneNode.setAttributeValue(STATE_ATTRIBUTE, state);
    }

    // Persist the docked pane locations
    GlobalRegistry().deleteXPath(RKEY_AUI_DOCKED_PANE_LOCATIONS);
    auto dockedPanesKey = GlobalRegistry().createKey(RKEY_AUI_DOCKED_PANE_LOCATIONS);

    for (const auto& [name, state] : _dockedPaneLocations)
    {
        auto paneNode = dockedPanesKey.createChild(PANE_NODE_NAME);
        paneNode.setAttributeValue(PANE_NAME_ATTRIBUTE, name);
        paneNode.setAttributeValue(STATE_ATTRIBUTE, state);
    }

    // Save property notebook stae
    _propertyNotebook->saveState();
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
    _auiMgr.SetPropertyNotebook(_propertyNotebook);

    auto orthoViewControl = GlobalUserInterface().findControl(UserControl::OrthoView);
    auto cameraControl = GlobalUserInterface().findControl(UserControl::Camera);
    assert(cameraControl);
    assert(orthoViewControl);

    // Add the camera and notebook to the left, as with the Embedded layout, and
    // the 2D view on the right
    wxSize size = topLevelParent->GetSize();
    size.Scale(0.5, 1.0);

    // Force a minimum width of the camera
    auto camera = cameraControl->createWidget(managedArea);
    auto previousMinSize = camera->GetMinSize();
    camera->SetMinSize(wxSize(600, -1));

    addPane(cameraControl->getControlName(), camera,
            DEFAULT_PANE_INFO(cameraControl->getDisplayName(), size).Left().Position(0));
    addPane(PROPERTIES_PANEL, _propertyNotebook,
            DEFAULT_PANE_INFO(_("Properties"), size).Left().Position(1).DestroyOnClose(false));
    addPane(orthoViewControl->getControlName(), orthoViewControl->createWidget(managedArea),
            DEFAULT_PANE_INFO(orthoViewControl->getDisplayName(), size).CenterPane());
    _auiMgr.Update();

    // Reset the camera min size
    camera->SetMinSize(previousMinSize);

    // Hide the camera toggle option for non-floating views
    GlobalMenuManager().setVisibility("main/view/cameraview", false);
    // Hide the console/texture browser toggles for non-floating/non-split views
    GlobalMenuManager().setVisibility("main/view/textureBrowser", false);
}

void AuiLayout::deactivate()
{
    // Get a reference to the managed window, it might be cleared by UnInit()
    auto managedWindow = _auiMgr.GetManagedWindow();

    // Unregister the AuiMgr from the event handlers of the managed window
    // otherwise we run into crashes during shutdown (#5586)
    _auiMgr.UnInit();

    managedWindow->Destroy();
}

void AuiLayout::createPane(const std::string& controlName, const std::string& paneName,
    const std::function<void(wxAuiPaneInfo&)>& setupPane)
{
    auto control = GlobalUserInterface().findControl(controlName);

    if (!control)
    {
        rError() << "Cannot find named control: " << controlName << std::endl;
        return;
    }

    auto managedWindow = _auiMgr.GetManagedWindow();

    auto pane = DEFAULT_PANE_INFO(control->getDisplayName(), MIN_SIZE);
    pane.name = paneName;

    // Create the widget and determine the best default size
    auto widget = control->createWidget(managedWindow);

    // Fit and determine the floating size automatically
    widget->Fit();

    // Some additional height for the window title bar
    pane.FloatingSize(widget->GetSize().x, widget->GetSize().y + 30);

    // Run client code to set up the pane properties, do this after setting the default size
    // since the client code may also try to restore any persisted size info
    setupPane(pane);

    if (!control->getIcon().empty())
    {
        pane.Icon(wxutil::GetLocalBitmap(control->getIcon()));
    }

    _auiMgr.AddPane(widget, pane);
    _panes.push_back({ paneName, controlName, widget });
}

void AuiLayout::createFloatingControl(const std::string& controlName)
{
    auto paneName = generateUniquePaneName(controlName);

    createPane(controlName, paneName, [&](auto& paneInfo)
    {
        setupFloatingPane(paneInfo);

        // Check for the default floating size
        auto defaultSettings = _defaultControlSettings.find(controlName);

        if (defaultSettings != _defaultControlSettings.end())
        {
            paneInfo.FloatingSize(defaultSettings->second.defaultFloatingWidth, 
                defaultSettings->second.defaultFloatingHeight);
        }

        restorePaneLocation(paneInfo);
    });

    auto& paneInfo = _auiMgr.GetPane(paneName);
    ensureControlIsActive(paneInfo.window);

    _auiMgr.Update();
}

void AuiLayout::registerControl(const std::string& controlName, const IMainFrame::ControlSettings& defaultSettings)
{
    _defaultControlSettings[controlName] = defaultSettings;
}

void AuiLayout::createControl(const std::string& controlName)
{
    auto defaultSettings = _defaultControlSettings.find(controlName);

    if (defaultSettings == _defaultControlSettings.end())
    {
        return;
    }

    switch (defaultSettings->second.location)
    {
    case IMainFrame::Location::PropertyPanel:
        _propertyNotebook->addControl(controlName);
        break;

    case IMainFrame::Location::FloatingWindow:
        createFloatingControl(controlName);
        break;
    }
}

void AuiLayout::ensureControlIsActive(wxWindow* control)
{
    if (auto dockablePanel = dynamic_cast<wxutil::DockablePanel*>(control); dockablePanel)
    {
        dockablePanel->activatePanel();
    }
}

void AuiLayout::ensureControlIsInactive(wxWindow* control)
{
    if (auto dockablePanel = dynamic_cast<wxutil::DockablePanel*>(control); dockablePanel)
    {
        dockablePanel->deactivatePanel();
    }
}

void AuiLayout::focusControl(const std::string& controlName)
{
    // Locate the control, is it loaded anywhere?
    if (!controlExists(controlName))
    {
        // Control is not loaded anywhere, check the named settings
        // Check the default settings if there's a control
        if (_defaultControlSettings.count(controlName) == 0)
        {
            throw cmd::ExecutionFailure(fmt::format(_("Cannot focus unknown control {0}"), controlName));
        }

        // Create the control in its default location
        createControl(controlName);
    }

    // Focus matching controls in the property notebook
    _propertyNotebook->focusControl(controlName);

    // Ensure the property panel is visible when focusing a tab
    auto& propertyPane = _auiMgr.GetPane(PROPERTIES_PANEL);
    if (!propertyPane.IsShown())
    {
        propertyPane.Show(true);
        _auiMgr.Update();
    }

    // Unset the focus of any wxPanels in floating windows
    for (auto p = _panes.begin(); p != _panes.end(); ++p)
    {
        if (p->controlName != controlName) continue;

        auto& paneInfo = _auiMgr.GetPane(p->control);

        // Show hidden panes
        if (!paneInfo.IsShown())
        {
            paneInfo.Show();
            ensureControlIsActive(p->control);
            _auiMgr.Update();
        }

        if (auto panel = wxDynamicCast(p->control, wxPanel); panel != nullptr)
        {
            panel->SetFocusIgnoringChildren();
        }
        break;
    }
}

void AuiLayout::toggleControlInPropertyPanel(const std::string& controlName)
{
    // If the control is a property tab, make it visible
    // If the notbeook is floating, toggling a visible tab also toggles the whole notebook
    auto& propertyPane = _auiMgr.GetPane(PROPERTIES_PANEL);

    if (propertyPane.IsFloating())
    {
        // Toggling a visible tab in the floating panel hides the whole notebook
        if (propertyPane.IsShown() && _propertyNotebook->controlIsVisible(controlName))
        {
            // Ensure active controls are deactivated
            _propertyNotebook->onNotebookPaneClosed();
            propertyPane.Hide();
            _auiMgr.Update();
            return;
        }

        // Tab is not visible yet, focus it
        _propertyNotebook->focusControl(controlName);

        // Ensure the floating pane is visible
        if (!propertyPane.IsShown())
        {
            propertyPane.Show(true);
            _auiMgr.Update();
            _propertyNotebook->onNotebookPaneRestored();
        }
    }
    else // Property panel is docked
    {
        // Focus matching controls in the property notebook
        _propertyNotebook->focusControl(controlName);

        // Ensure the non-floating property panel is visible
        if (!propertyPane.IsShown())
        {
            propertyPane.Show(true);
            _auiMgr.Update();
            _propertyNotebook->onNotebookPaneRestored();
        }
    }
}

void AuiLayout::toggleControl(const std::string& controlName)
{
    // Check the default settings if there's a control
    if (_defaultControlSettings.count(controlName) == 0)
    {
        throw cmd::ExecutionFailure(fmt::format(_("Cannot toggle unknown control {0}"), controlName));
    }

    // Locate the control, is it loaded anywhere?
    if (!controlExists(controlName))
    {
        // Control is not loaded anywhere; focusControl will create it in its default location
        focusControl(controlName);
        return;
    }

    // Control exists, we can toggle

    // Controls in the property panel receive special treatment on toggling
    if (_propertyNotebook->controlExists(controlName))
    {
        toggleControlInPropertyPanel(controlName);
        return;
    }

    // If it's a docked pane, toggle its visibility
    for (auto p = _panes.begin(); p != _panes.end(); ++p)
    {
        if (p->controlName != controlName) continue;

        auto& paneInfo = _auiMgr.GetPane(p->control);

        // Show/hide, unless it's the center pane
        if (!isCenterPane(paneInfo))
        {
            if (paneInfo.IsShown())
            {
                savePaneLocation(paneInfo);
                paneInfo.Hide();
                ensureControlIsInactive(p->control);
            }
            else
            {
                paneInfo.Show();
                restorePaneLocation(paneInfo);

                if (paneInfo.dock_direction == wxAUI_DOCK_LEFT || paneInfo.dock_direction == wxAUI_DOCK_RIGHT)
                {
                    // Set the minimum width of this restored pane otherwise the
                    // wxAuiManager::Update() routine is resetting the pane to its minimum dimensions
                    paneInfo.MinSize(paneInfo.rect.GetSize().x, -1);
                }

                ensureControlIsActive(p->control);
            }
            
            _auiMgr.Update();

            // Reset the mininum size again to not interfere with any drag operations
            if (paneInfo.IsDocked())
            {
                paneInfo.MinSize(MIN_SIZE);
            }
        }

        break;
    }
}

void AuiLayout::removeNonOrthoCenterPanes()
{
    for (auto p = _panes.begin(); p != _panes.end();)
    {
        auto& paneInfo = _auiMgr.GetPane(p->paneName);

        if (!isCenterPane(paneInfo) || string::starts_with(paneInfo.name.ToStdString(), UserControl::OrthoView))
        {
            ++p;
            continue;
        }

        ensureControlIsInactive(paneInfo.window);
        paneInfo.DestroyOnClose(true);
        _auiMgr.ClosePane(paneInfo);
        _panes.erase(p++);
    }
}

void AuiLayout::toggleMainControl(const std::string& controlName)
{
    // Check the center pane to see what's currently in there
    for (auto p = _panes.begin(); p != _panes.end(); ++p)
    {
        auto& paneInfo = _auiMgr.GetPane(p->paneName);

        if (!isCenterPane(paneInfo) || !paneInfo.IsShown()) continue;

        // If the existing pane has already the requested control, switch back to ortho
        auto newControlName = string::starts_with(paneInfo.name.ToStdString(), controlName) ? UserControl::OrthoView : controlName;

        if (newControlName == paneInfo.name) return; // nothing to do

        auto control = GlobalUserInterface().findControl(newControlName);

        if (!control)
        {
            throw std::invalid_argument("Cannot make " + newControlName + " to a main control");
        }

        rMessage() << "Swapping " << newControlName << " with the existing main control " <<
            paneInfo.name.ToStdString() << std::endl;

        // Find the hidden OrthoView pane
        auto& existingOrthoView = _auiMgr.GetPane(UserControl::OrthoView);

        if (newControlName == UserControl::OrthoView)
        {
            existingOrthoView.Show();
            ensureControlIsActive(existingOrthoView.window);

            removeNonOrthoCenterPanes();
        }
        else
        {
            // Hide the existing OrthoView pane
            existingOrthoView.Hide();
            ensureControlIsInactive(existingOrthoView.window);

            // Close all other center panes, we don't need two main panels
            removeNonOrthoCenterPanes();

            // Add the new control as center pane
            auto newWidget = control->createWidget(_auiMgr.GetManagedWindow());
            addPane(newControlName, newWidget, DEFAULT_PANE_INFO(control->getDisplayName(), MIN_SIZE).CenterPane());
            ensureControlIsActive(newWidget);
        }
        
        _auiMgr.Update();
        break;
    }
}

void AuiLayout::savePaneLocation(wxAuiPaneInfo& pane)
{
    if (pane.IsFloating())
    {
        _floatingPaneLocations[pane.name.ToStdString()] = _auiMgr.SavePaneInfo(pane).ToStdString();
    }
    else
    {
        _dockedPaneLocations[pane.name.ToStdString()] = _auiMgr.SavePaneInfo(pane).ToStdString();
    }
}

void AuiLayout::restorePaneLocation(wxAuiPaneInfo& pane)
{
    // Check if we got existing size information for this floating pane
    const auto& locations = pane.IsFloating() ? _floatingPaneLocations : _dockedPaneLocations;

    auto existingSizeInfo = locations.find(pane.name.ToStdString());

    if (existingSizeInfo != locations.end())
    {
        _auiMgr.LoadPaneInfo(existingSizeInfo->second, pane);
    }
}

void AuiLayout::restoreStateFromRegistry()
{
    // Check the saved version
    if (registry::getValue<int>(RKEY_AUI_LAYOUT_VERSION) != AuiLayoutVersion)
    {
        rMessage() << "No compatible AUI layout state information found in registry" << std::endl;

        // We still need to set up the default pages of the notebook
        _propertyNotebook->restoreDefaultState();
        return;
    }

    _floatingPaneLocations.clear();
    for (const auto& node : GlobalRegistry().findXPath(RKEY_AUI_FLOATING_PANE_LOCATIONS + "//*"))
    {
        _floatingPaneLocations[node.getAttributeValue(PANE_NAME_ATTRIBUTE)] = node.getAttributeValue(STATE_ATTRIBUTE);
    }

    _dockedPaneLocations.clear();
    for (const auto& node : GlobalRegistry().findXPath(RKEY_AUI_DOCKED_PANE_LOCATIONS + "//*"))
    {
        _dockedPaneLocations[node.getAttributeValue(PANE_NAME_ATTRIBUTE)] = node.getAttributeValue(STATE_ATTRIBUTE);
    }

    // Restore all missing panes, this has to be done before the perspective is restored
    for (const auto& node : GlobalRegistry().findXPath(RKEY_AUI_PANES + "//*"))
    {
        if (node.getName() != PANE_NODE_NAME) continue;

        auto controlName = node.getAttributeValue(CONTROL_NAME_ATTRIBUTE);
        auto paneName = node.getAttributeValue(PANE_NAME_ATTRIBUTE);

        if (paneNameExists(paneName)) continue; // this one already exists

        createPane(controlName, paneName, setupFloatingPane);
    }

    // Restore the property notebook state (will fall back to default if nothing found)
    _propertyNotebook->restoreState();

    // Nasty hack to get the panes sized properly. Since BestSize() is
    // completely ignored (at least on Linux), we have to add the panes with a
    // large *minimum* size and then reset this size after the initial addition.
    for (const auto& info : _panes)
    {
        _auiMgr.GetPane(info.control).MinSize(MIN_SIZE);
    }

    _auiMgr.Update();

    // If we have a stored perspective, load it
    auto storedPersp = registry::getValue<std::string>(RKEY_AUI_PERSPECTIVE);

    if (!storedPersp.empty())
    {
        _auiMgr.LoadPerspective(storedPersp);
    }

    // Make sure the properties panel can be closed (the flag might still be set on the stored perspective)
    auto& propertyPane = _auiMgr.GetPane(PROPERTIES_PANEL);
    propertyPane.CloseButton(true).DestroyOnClose(false);

    ensureVisibleCenterPane();

    // After restoring the perspective, ensure all visible panes are active
    for (const auto& info : _panes)
    {
        auto& paneInfo = _auiMgr.GetPane(info.control);

        if (paneInfo.IsShown())
        {
            ensureControlIsActive(paneInfo.window);
        }
        else
        {
            ensureControlIsInactive(paneInfo.window);
        }
    }

    // Make sure the currently shown tab is made active after everything is restored
    // this way tabs can work with the restored window sizes
    if (propertyPane.IsShown())
    {
        _propertyNotebook->onNotebookPaneRestored();
    }
}

void AuiLayout::ensureVisibleCenterPane()
{
    // Ensure if we have a visible center pane
    bool hasVisibleCenterPane = false;
    wxAuiPaneInfo* centerOrthoPane = nullptr;

    for (const auto& info : _panes)
    {
        auto& paneInfo = _auiMgr.GetPane(info.paneName);

        if (!isCenterPane(paneInfo)) continue;

        if (paneInfo.IsShown())
        {
            hasVisibleCenterPane = true;
        }

        if (paneInfo.name == UserControl::OrthoView)
        {
            centerOrthoPane = &paneInfo;
        }
    }

    if (!hasVisibleCenterPane && centerOrthoPane != nullptr)
    {
        // Set the default ortho view center pane to visible
        centerOrthoPane->Show();
        _auiMgr.Update();
    }
}

} // namespace ui
