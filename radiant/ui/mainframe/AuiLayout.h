#pragma once

#include <memory>
#include "ui/imainframelayout.h"

#include "camera/CamWnd.h"

#include <wx/aui/aui.h>

#include "PropertyNotebook.h"

namespace ui
{

constexpr const char* const AUI_LAYOUT_NAME = "Dockable";

/// Layout based on wxWidgets AUI (dock widget interface)
class AuiLayout : public IMainFrameLayout
{
    // Main AUI manager
    wxAuiManager _auiMgr;
    PropertyNotebook* _propertyNotebook;

    struct PaneInfo
    {
        std::string controlName;
        wxWindow* control;
    };

    // List of panes managed by the AUI manager
    std::list<PaneInfo> _panes;

public:
    AuiLayout();

    PropertyNotebook* getNotebook();

	// IMainFrameLayout implementation
	std::string getName() override;
	void activate() override;
	void deactivate() override;
	void toggleFullscreenCameraView() override;
	void restoreStateFromRegistry() override;
    void createFloatingControl(const std::string& controlName) override;

	// The creation function, needed by the mainframe layout manager
	static std::shared_ptr<AuiLayout> CreateInstance();

private:
    // Add a pane to the wxAuiManager and store it in the list
    void addPane(const std::string& name, wxWindow* window, const wxAuiPaneInfo& info);
    void onPaneClose(wxAuiManagerEvent& ev);
};

} // namespace
