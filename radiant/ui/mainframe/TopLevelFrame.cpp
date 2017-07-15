#include "TopLevelFrame.h"

#include "itextstream.h"
#include "i18n.h"
#include "iuimanager.h"
#include "ui/menu/FiltersMenu.h"
#include "map/Map.h"
#include <wx/artprov.h>
#include <wx/toolbar.h>
#include <wx/menu.h>
#include <wx/app.h>

namespace ui
{

TopLevelFrame::TopLevelFrame() :
	wxFrame(NULL, wxID_ANY, wxT("DarkRadiant")),
	_topLevelContainer(NULL),
	_mainContainer(NULL)
{
	_topLevelContainer = new wxBoxSizer(wxVERTICAL);
	SetSizer(_topLevelContainer);

	wxMenuBar* menuBar = createMenuBar();

	if (menuBar != nullptr)
	{
		SetMenuBar(menuBar);
	}

	// Instantiate the ToolbarManager and retrieve the view toolbar widget
	IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();

	wxToolBar* viewToolbar = tbCreator.getToolbar("view", this);

	if (viewToolbar != NULL)
	{
		_toolbars[IMainFrame::TOOLBAR_HORIZONTAL] = viewToolbar;

		// Pack it into the main window
		_topLevelContainer->Add(_toolbars[IMainFrame::TOOLBAR_HORIZONTAL].get(), 0, wxEXPAND);
	}
	else
	{
		rWarning() << "TopLevelFrame: Cannot instantiate view toolbar!" << std::endl;
	}

	_mainContainer = new wxBoxSizer(wxHORIZONTAL);
	_topLevelContainer->Add(_mainContainer, 1, wxEXPAND);

	// Get the edit toolbar widget
	wxToolBar* editToolbar = tbCreator.getToolbar("edit", this);

	if (editToolbar != NULL)
	{
		_toolbars[IMainFrame::TOOLBAR_VERTICAL] = editToolbar;

		// Pack it into the main window
		_mainContainer->Add(_toolbars[IMainFrame::TOOLBAR_VERTICAL].get(), 0, wxEXPAND);
	}
	else
	{
		rWarning() << "MainFrame: Cannot instantiate edit toolbar!" << std::endl;
	}

	wxWindow* statusBar = GlobalUIManager().getStatusBarManager().getStatusBar();

	statusBar->Reparent(this);
	statusBar->SetCanFocus(false);
	_topLevelContainer->Add(statusBar, 0, wxEXPAND);

	// Set the window icon
	wxIcon appIcon;
	appIcon.CopyFromBitmap(wxArtProvider::GetBitmap(
		GlobalUIManager().ArtIdPrefix() + "darkradiant_icon_64x64.png"));
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
	ToolbarMap::const_iterator found = _toolbars.find(type);

	return (found != _toolbars.end()) ? found->second.get() : nullptr;
}

wxMenuBar* TopLevelFrame::createMenuBar()
{
	// Create the Filter menu entries before adding the menu bar
    FiltersMenu::addItemsToMainMenu();

    // Return the "main" menubar from the UIManager
	return GlobalUIManager().getMenuManager().getMenuBar("main");
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
