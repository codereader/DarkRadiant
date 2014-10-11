#include "TopLevelFrame.h"

#include "ieventmanager.h"
#include "itextstream.h"
#include "i18n.h"
#include "iuimanager.h"
#include "KeyEventPropagator.h"
#include "ui/menu/FiltersMenu.h"
#include "map/Map.h"
#include <wx/artprov.h>
#include <wx/toolbar.h>
#include <wx/menu.h>
#include <wx/app.h>

namespace ui
{

BEGIN_EVENT_TABLE(TopLevelFrame, wxFrame)
	EVT_MOUSEWHEEL(TopLevelFrame::redirectMouseWheelToWindowBelowCursor)
	EVT_CLOSE(TopLevelFrame::onCloseWindow)
END_EVENT_TABLE()

TopLevelFrame::TopLevelFrame() :
	wxFrame(NULL, wxID_ANY, wxT("DarkRadiant")),
	_topLevelContainer(NULL),
	_mainContainer(NULL),
	_keyEventFilter(new KeyEventPropagationFilter)
{
	_topLevelContainer = new wxBoxSizer(wxVERTICAL);
	SetSizer(_topLevelContainer);

	wxMenuBar* menuBar = createMenuBar();

	if (menuBar != NULL)
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
		_topLevelContainer->Add(_toolbars[IMainFrame::TOOLBAR_HORIZONTAL], 0, wxEXPAND);
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
		_mainContainer->Add(_toolbars[IMainFrame::TOOLBAR_VERTICAL], 0, wxEXPAND);
	}
	else
	{
		rWarning() << "MainFrame: Cannot instantiate edit toolbar!" << std::endl;
	}

	GlobalEventManager().connect(*this);

	wxWindow* statusBar = GlobalUIManager().getStatusBarManager().getStatusBar();

	statusBar->Reparent(this);
	statusBar->SetCanFocus(false);
	_topLevelContainer->Add(statusBar, 0, wxEXPAND);

	// Set the window icon
	wxIcon appIcon;
	appIcon.CopyFromBitmap(wxArtProvider::GetBitmap(
		GlobalUIManager().ArtIdPrefix() + "darkradiant_icon_64x64.png"));
	SetIcon(appIcon);
}

TopLevelFrame::~TopLevelFrame()
{
	GlobalEventManager().disconnect(*this);
}

wxToolBar* TopLevelFrame::getToolbar(IMainFrame::Toolbar type)
{
	ToolbarMap::const_iterator found = _toolbars.find(type);

	return (found != _toolbars.end()) ? found->second : NULL;
}

wxMenuBar* TopLevelFrame::createMenuBar()
{
	// Create the Filter menu entries before adding the menu bar
    FiltersMenu::addItemsToMainMenu();

    // Return the "main" menubar from the UIManager
	return dynamic_cast<wxMenuBar*>(GlobalUIManager().getMenuManager().get("main"));
}

void TopLevelFrame::onCloseWindow(wxCloseEvent& ev)
{
	if (!ev.CanVeto() || GlobalMap().askForSave(_("Exit Radiant")))
	{
		wxTheApp->ExitMainLoop();
		return;
	}

	ev.Veto();
}

void TopLevelFrame::redirectMouseWheelToWindowBelowCursor(wxMouseEvent& ev)
{
	wxPoint mousePos = wxGetMousePosition();
	wxWindow* windowAtPoint = wxFindWindowAtPointer(mousePos);

	if (windowAtPoint) 
	{
		windowAtPoint->GetEventHandler()->AddPendingEvent(ev);
	}
}

wxBoxSizer* TopLevelFrame::getMainContainer()
{
	return _mainContainer;
}

} // namespace
