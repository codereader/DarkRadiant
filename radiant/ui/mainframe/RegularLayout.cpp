#include "RegularLayout.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include "gtkutil/FramedWidget.h"

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

#include <gtkmm/paned.h>

namespace ui {

	namespace
	{
		const std::string RKEY_REGULAR_ROOT = "user/ui/mainFrame/regular";
		const std::string RKEY_REGULAR_TEMP_ROOT = RKEY_REGULAR_ROOT + "/temp";
	}

RegularLayout::RegularLayout(bool regularLeft) :
	_regularLeft(regularLeft)
{}

std::string RegularLayout::getName() {
	return REGULAR_LAYOUT_NAME;
}

void RegularLayout::activate() {

	const Glib::RefPtr<Gtk::Window>& parent = GlobalMainFrame().getTopLevelWindow();

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd();
	 // greebo: The mainframe window acts as parent for the camwindow
	_camWnd->setContainer(parent);
	// Pack in the camera window
	Gtk::Widget* camWindow = Gtk::manage(new gtkutil::FramedWidget(*_camWnd->getWidget()));

	// Allocate a new OrthoView and set its ViewType to XY
	XYWndPtr xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
    xyWnd->setViewType(XY);
    // Create a framed window out of the view's internal widget
	Gtk::Widget* xyView = Gtk::manage(new gtkutil::FramedWidget(*xyWnd->getWidget()));

	// Create the texture window
	Gtk::Frame* texWindow = Gtk::manage(new gtkutil::FramedWidget(
		*GlobalTextureBrowser().constructWindow(parent)
	));

	// Now pack those widgets into the paned widgets
	_regular.texCamPane = Gtk::manage(new Gtk::VPaned);

	// First, pack the texwindow and the camera
	_regular.texCamPane->pack1(*camWindow, true, true); // allow shrinking
	_regular.texCamPane->pack2(*texWindow, true, true); // allow shrinking

    // Depending on the viewstyle, pack the xy left or right
	_regular.horizPane.reset(new Gtk::HPaned);

    if (_regularLeft)
	{
		_regular.horizPane->pack1(*_regular.texCamPane, true, true); // allow shrinking
		_regular.horizPane->pack2(*xyView, true, true); // allow shrinking
    }
    else
	{
		// This is "regular", put the xyview to the left
		_regular.horizPane->pack1(*xyView, true, true); // allow shrinking
		_regular.horizPane->pack2(*_regular.texCamPane, true, true); // allow shrinking
    }

	// Retrieve the main container of the main window
	Gtk::Container* mainContainer = GlobalMainFrame().getMainContainer();
	mainContainer->add(*_regular.horizPane);

	// Set some default values for the width and height
	_regular.horizPane->set_position(500);
	_regular.texCamPane->set_position(350);

	// Connect the pane position trackers
	_regular.posHPane.connect(_regular.horizPane.get());
	_regular.posTexCamPane.connect(_regular.texCamPane);

	// Now attempt to load the paned positions from the registry
	restoreStateFromPath(RKEY_REGULAR_ROOT);

    GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload
	// the position info from the Registry once again.
	GlobalEntityInspector().restoreSettings();

	GlobalGroupDialog().hideDialogWindow();

	mainContainer->show_all();

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

	// Delete all active views
	GlobalXYWndManager().destroyViews();

	// Delete the CamWnd
	_camWnd = CamWndPtr();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	GlobalTextureBrowser().destroyWindow();

	// Destroy the widget, so it gets removed from the main container
	_regular.horizPane.reset();
}

void RegularLayout::maximiseCameraSize()
{
	// Save the current state to the registry
	saveStateToPath(RKEY_REGULAR_TEMP_ROOT);

	// Maximise the camera
	if (_regularLeft)
	{
		_regular.posHPane.applyMaxPosition();
	}
	else
	{
		_regular.posHPane.applyMinPosition();
	}

	_regular.posTexCamPane.applyMaxPosition();
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
	// Now load the paned positions from the registry
	if (GlobalRegistry().keyExists(path + "/pane[@name='horizontal']"))
	{
		_regular.posHPane.loadFromPath(path + "/pane[@name='horizontal']");
		_regular.posHPane.applyPosition();
	}

	if (GlobalRegistry().keyExists(path + "/pane[@name='texcam']"))
	{
		_regular.posTexCamPane.loadFromPath(path + "/pane[@name='texcam']");
		_regular.posTexCamPane.applyPosition();
	}
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
RegularLayoutPtr RegularLayout::CreateRegularLeftInstance() {
	return RegularLayoutPtr(new RegularLayout(true));
}

RegularLayoutPtr RegularLayout::CreateRegularInstance() {
	return RegularLayoutPtr(new RegularLayout(false));
}

} // namespace ui
