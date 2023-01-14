#pragma once

#include <memory>

#include "camera/CamWnd.h"
#include "ui/mainframe/MainFrame.h"

#include "AuiManager.h"
#include "PropertyNotebook.h"

namespace ui
{

constexpr const char* const AUI_LAYOUT_NAME = "Dockable";

class AuiFloatingFrame;

/// Layout based on wxWidgets AUI (dock widget interface)
class AuiLayout
{
    // Main AUI manager
    AuiManager _auiMgr;
    PropertyNotebook* _propertyNotebook;

    struct PaneInfo
    {
        std::string paneName;
        std::string controlName;
        wxWindow* control;
    };

    // List of panes managed by the AUI manager
    std::list<PaneInfo> _panes;

    std::map<std::string, IMainFrame::ControlSettings> _defaultControlSettings;

    // Stored floating window locations
    std::map<std::string, std::string> _floatingPaneLocations;

    // Stored last known locations of docked panes
    std::map<std::string, std::string> _dockedPaneLocations;

public:
    AuiLayout();

	void activate();
	void deactivate();
	void saveStateToRegistry();
	void restoreStateFromRegistry();
    void createFloatingControl(const std::string& controlName);

    void registerControl(const std::string& controlName, const IMainFrame::ControlSettings& defaultSettings);

    // Creates the named control at its registered default location
    void createControl(const std::string& controlName);
    void focusControl(const std::string& controlName);
    void toggleControl(const std::string& controlName);
    void toggleMainControl(const std::string& controlName);

    void ensureControlIsActive(wxWindow* control);
    void ensureControlIsInactive(wxWindow* control);

    // Internally used by the AuiManager implementation
    void convertFloatingPaneToPropertyTab(AuiFloatingFrame* floatingWindow);

private:
    // Add a pane to the wxAuiManager and store it in the list
    void addPane(const std::string& controlName, wxWindow* window, const wxAuiPaneInfo& info);
    void addPane(const std::string& controlName, const std::string& paneName, wxWindow* window, const wxAuiPaneInfo& info);

    void createPane(const std::string& controlName, const std::string& paneName,
        const std::function<void(wxAuiPaneInfo&)>& setupPane);

    void toggleControlInPropertyPanel(const std::string& controlName);

    void onPaneClose(wxAuiManagerEvent& ev);
    void handlePaneClosed(wxAuiPaneInfo& paneInfo);
    void removeNonOrthoCenterPanes();

    void savePaneLocation(wxAuiPaneInfo& paneInfo);
    void restorePaneLocation(wxAuiPaneInfo& paneInfo);

    bool paneNameExists(const std::string& name) const;

    // Returns true if the control is loaded in the notebook or in a pane
    bool controlExists(const std::string& controlName) const;

    std::string generateUniquePaneName(const std::string& controlName);

    void convertPaneToPropertyTab(const std::string& paneName);
    void ensureVisibleCenterPane();
};

} // namespace
