#include "TopLevelFrame.h"

#include "itextstream.h"
#include "i18n.h"
#include "version.h"
#include "ui/imenumanager.h"
#include "ui/istatusbarmanager.h"
#include "ui/itoolbarmanager.h"
#include "wxutil/Bitmap.h"
#include <wx/toolbar.h>
#include <wx/menu.h>
#include <wx/app.h>
#include <wx/stattext.h>

namespace ui
{

TopLevelFrame::TopLevelFrame() :
	wxFrame(nullptr, wxID_ANY, wxT("DarkRadiant")),
	_topLevelContainer(nullptr),
	_mainContainer(nullptr)
{
	_topLevelContainer = new wxBoxSizer(wxVERTICAL);
	SetSizer(_topLevelContainer);

	wxMenuBar* menuBar = createMenuBar();

	if (menuBar != nullptr)
	{
		SetMenuBar(menuBar);
	}

	// Instantiate the ToolbarManager and retrieve the view toolbar widget
	auto* viewToolbar = GlobalToolBarManager().createToolbar("view", this);

	if (viewToolbar)
	{
		_toolbars[IMainFrame::Toolbar::TOP] = viewToolbar;

        // Add a version label to the right
        auto versionToolbar = new wxToolBar(this, wxID_ANY);
        auto versionLabel = new wxStaticText(versionToolbar, wxID_ANY, std::string(RADIANT_VERSION) + " ");
        versionToolbar->AddControl(versionLabel);
        versionToolbar->Realize();

        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        
        sizer->Add(_toolbars[IMainFrame::Toolbar::TOP].get(), 1);
        sizer->Add(versionToolbar, 0, wxEXPAND);

		_topLevelContainer->Add(sizer, 0, wxEXPAND);
	}
	else
	{
		rWarning() << "TopLevelFrame: Cannot instantiate view toolbar!" << std::endl;
	}

	_mainContainer = new wxBoxSizer(wxHORIZONTAL);
	_topLevelContainer->Add(_mainContainer, 1, wxEXPAND);

	// Get the edit toolbar widget
	auto* editToolbar = GlobalToolBarManager().createToolbar("edit", this);

	if (editToolbar)
	{
		_toolbars[IMainFrame::Toolbar::LEFT] = editToolbar;

		// Pack it into the main window
		_mainContainer->Add(_toolbars[IMainFrame::Toolbar::LEFT].get(), 0, wxEXPAND);
	}
	else
	{
		rWarning() << "MainFrame: Cannot instantiate edit toolbar!" << std::endl;
	}

	wxWindow* statusBar = GlobalStatusBarManager().getStatusBar();

	statusBar->Reparent(this);
	statusBar->SetCanFocus(false);
	_topLevelContainer->Add(statusBar, 0, wxEXPAND);

	// Set the window icon
	wxIcon appIcon;
	appIcon.CopyFromBitmap(wxutil::GetLocalBitmap("darkradiant_icon_64x64.png"));
	SetIcon(appIcon);

    // Redirect scroll events to the window below the cursor
    _scrollEventFilter.reset(new wxutil::ScrollEventPropagationFilter);

#if (wxMAJOR_VERSION >= 3) && (wxMINOR_VERSION < 1)
	// In wxWidgets < 3.1.0 we don't receive the wxEVT_MENU_OPEN event on 
	// the menu itself (only on the toplevel frame), so let's propagate it
	Connect(wxEVT_MENU_OPEN, wxMenuEventHandler(TopLevelFrame::onMenuOpenClose), nullptr, this);
#endif
}

wxToolBar* TopLevelFrame::getToolbar(IMainFrame::Toolbar type)
{
	auto found = _toolbars.find(type);

	return found != _toolbars.end() ? found->second.get() : nullptr;
}

bool TopLevelFrame::Destroy()
{
	// Clear out any references to toolbars while the modules are still loaded
	_toolbars.clear();

	return wxFrame::Destroy();
}

wxMenuBar* TopLevelFrame::createMenuBar()
{
    // Return the "main" menubar from the UIManager
	return GlobalMenuManager().getMenuBar("main");
}

wxBoxSizer* TopLevelFrame::getMainContainer()
{
	return _mainContainer;
}

void TopLevelFrame::onMenuOpenClose(wxMenuEvent& ev)
{
	if (GetMenuBar() != nullptr)
	{
		if (!GetMenuBar()->HandleWindowEvent(ev))
		{
			ev.Skip();
		}
	}
}

} // namespace
