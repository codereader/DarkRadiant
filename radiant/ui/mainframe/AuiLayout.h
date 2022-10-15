#pragma once

#include <memory>
#include "wxutil/PanedPosition.h"
#include "ui/imainframelayout.h"

#include "camera/CamWnd.h"
#include "wxutil/Splitter.h"

#include <wx/aui/aui.h>

namespace ui
{

#define AUI_LAYOUT_NAME "Dockable"

class AuiLayout;
typedef std::shared_ptr<AuiLayout> AuiLayoutPtr;

/// Layout based on wxWidgets AUI (dock widget interface)
class AuiLayout: public IMainFrameLayout
{
    // Main AUI manager
    wxAuiManager _auiMgr;

    // List of panes managed by the AUI manager
    std::list<wxWindow*> _panes;

    // Main constructor
    AuiLayout();

    // Add a pane to the wxAuiManager and store it in the list
    void addPane(wxWindow* window, const wxAuiPaneInfo& info);

public:
	// IMainFrameLayout implementation
	std::string getName() override;
	void activate() override;
	void deactivate() override;
	void toggleFullscreenCameraView() override;
	void restoreStateFromRegistry() override;
    void createFloatingControl(const std::string& controlName) override;

	// The creation function, needed by the mainframe layout manager
	static AuiLayoutPtr CreateInstance();
};

} // namespace ui
