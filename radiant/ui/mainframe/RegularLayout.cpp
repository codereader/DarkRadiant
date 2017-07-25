#include "RegularLayout.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include "camera/GlobalCamera.h"
#include "camera/CamWnd.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

#include <wx/splitter.h>
#include <wx/sizer.h>

namespace ui
{

namespace
{
	const std::string RKEY_REGULAR_ROOT = "user/ui/mainFrame/regular";
	const std::string RKEY_REGULAR_TEMP_ROOT = RKEY_REGULAR_ROOT + "/temp";
}

RegularLayout::RegularLayout(bool regularLeft) :
	_regularLeft(regularLeft)
{}

std::string RegularLayout::getName()
{
	return REGULAR_LAYOUT_NAME;
}

void RegularLayout::activate()
{
	wxFrame* topLevelParent = GlobalMainFrame().getWxTopLevelWindow();

	// Main splitter
	_regular.horizPane = new wxSplitterWindow(topLevelParent, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, 
		wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "RegularHorizPane");

	_regular.horizPane->SetSashGravity(0.5);
	_regular.horizPane->SetSashPosition(500);
    _regular.horizPane->SetMinimumPaneSize(1); // disallow unsplitting

	GlobalMainFrame().getWxMainContainer()->Add(_regular.horizPane, 1, wxEXPAND);

	// Allocate a new OrthoView and set its ViewType to XY
	XYWndPtr xywnd = GlobalXYWnd().createEmbeddedOrthoView(XY, _regular.horizPane);

	// Texture/Camera Pane
	_regular.texCamPane = new wxSplitterWindow(_regular.horizPane, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, 
		wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "RegularTexCamPane");

	_regular.texCamPane->SetSashGravity(0.5);
	_regular.texCamPane->SetSashPosition(350);
    _regular.texCamPane->SetMinimumPaneSize(1); // disallow unsplitting

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd(_regular.texCamPane);

	// Texture Window
	wxWindow* texBrowser = new TextureBrowser(_regular.texCamPane);

	_regular.texCamPane->SplitHorizontally(_camWnd->getMainWidget(), texBrowser);

	if (_regularLeft)
	{
		_regular.horizPane->SplitVertically(_regular.texCamPane, xywnd->getGLWidget());
    }
    else
	{
		// This is "regular", put the xyview to the left
		_regular.horizPane->SplitVertically(xywnd->getGLWidget(), _regular.texCamPane);
    }

	// Connect the pane position trackers
	_regular.posHPane.connect(_regular.horizPane);
	_regular.posTexCamPane.connect(_regular.texCamPane);

	// Now attempt to load the paned positions from the registry
	restoreStateFromPath(RKEY_REGULAR_ROOT);

	GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload
	// the position info from the Registry once again.
	GlobalEntityInspector().restoreSettings();

	GlobalGroupDialog().hideDialogWindow();

	topLevelParent->Layout();

	// Hide the camera toggle option for non-floating views
    GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", false);
	// Hide the console/texture browser toggles for non-floating/non-split views
	GlobalUIManager().getMenuManager().setVisibility("main/view/textureBrowser", false);
}

void RegularLayout::deactivate()
{
	// Show the camera toggle option again
    GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", true);
	GlobalUIManager().getMenuManager().setVisibility("main/view/textureBrowser", true);

	// Remove all previously stored pane information
	GlobalRegistry().deleteXPath(RKEY_REGULAR_ROOT + "//pane");

	// Save pane info
	saveStateToPath(RKEY_REGULAR_ROOT);

	// Disconnect before destroying stuff
	_regular.posHPane.disconnect();
	_regular.posTexCamPane.disconnect();

	// Delete all active views
	GlobalXYWndManager().destroyViews();

	// Delete the CamWnd
	_camWnd.reset();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	delete _regular.horizPane;

	_regular.horizPane = nullptr;
	_regular.texCamPane = nullptr;
}

void RegularLayout::maximiseCameraSize()
{
	// Save the current state to the registry
	saveStateToPath(RKEY_REGULAR_TEMP_ROOT);

	// Maximise the camera
	if (_regularLeft)
	{
		_regular.horizPane->SetSashPosition(0);
	}
	else
	{
		_regular.horizPane->SetSashPosition(2000000);
	}
}

void RegularLayout::restorePanePositions()
{
	// Restore state
	restoreStateFromPath(RKEY_REGULAR_TEMP_ROOT);

	// Remove all previously stored pane information
	GlobalRegistry().deleteXPath(RKEY_REGULAR_TEMP_ROOT);
}

void RegularLayout::restoreStateFromPath(const std::string& path)
{
    // Trigger a proper resize event before setting the sash position
    GlobalMainFrame().getWxTopLevelWindow()->SendSizeEvent();
    wxTheApp->Yield();

	// Now load the paned positions from the registry
	if (GlobalRegistry().keyExists(path + "/pane[@name='horizontal']"))
	{
		_regular.posHPane.loadFromPath(path + "/pane[@name='horizontal']");
	}

	if (GlobalRegistry().keyExists(path + "/pane[@name='texcam']"))
	{
		_regular.posTexCamPane.loadFromPath(path + "/pane[@name='texcam']");
	}
}

void RegularLayout::restoreStateFromRegistry()
{
	restoreStateFromPath(RKEY_REGULAR_ROOT);
}

void RegularLayout::saveStateToPath(const std::string& path)
{
	GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
	_regular.posHPane.saveToPath(path + "/pane[@name='horizontal']");

	GlobalRegistry().createKeyWithName(path, "pane", "texcam");
	_regular.posTexCamPane.saveToPath(path + "/pane[@name='texcam']");
}

void RegularLayout::toggleFullscreenCameraView()
{
	if (GlobalRegistry().keyExists(RKEY_REGULAR_TEMP_ROOT))
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
RegularLayoutPtr RegularLayout::CreateRegularLeftInstance()
{
	return RegularLayoutPtr(new RegularLayout(true));
}

RegularLayoutPtr RegularLayout::CreateRegularInstance()
{
	return RegularLayoutPtr(new RegularLayout(false));
}

} // namespace ui
