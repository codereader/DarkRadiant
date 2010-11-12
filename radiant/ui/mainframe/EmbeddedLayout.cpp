#include "EmbeddedLayout.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include "gtkutil/FramedWidget.h"

#include <gtkmm/paned.h>
#include <gtkmm/box.h>

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

namespace ui
{

	namespace
	{
		const std::string RKEY_EMBEDDED_ROOT = "user/ui/mainFrame/embedded";
		const std::string RKEY_EMBEDDED_TEMP_ROOT = RKEY_EMBEDDED_ROOT + "/temp";
	}

std::string EmbeddedLayout::getName() {
	return EMBEDDED_LAYOUT_NAME;
}

void EmbeddedLayout::activate()
{
	// Get the toplevel window
	const Glib::RefPtr<Gtk::Window>& parent = GlobalMainFrame().getTopLevelWindow();

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd();
	 // greebo: The mainframe window acts as parent for the camwindow
	_camWnd->setContainer(parent);
	// Pack in the camera window
	Gtk::Frame* camWindow = Gtk::manage(new gtkutil::FramedWidget(*_camWnd->getWidget()));

	// Allocate a new OrthoView and set its ViewType to XY
	XYWndPtr xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
    xyWnd->setViewType(XY);

    // Create a framed window out of the view's internal widget
	Gtk::Frame* xyView = Gtk::manage(new gtkutil::FramedWidget(*xyWnd->getWidget()));

	// Detach the notebook from the groupdialog to fit it into our pane
	Gtk::VBox* groupPane = Gtk::manage(new Gtk::VBox(false, 0));

	// Now pack those widgets into the paned widgets

	// First, pack the groupPane and the camera
	_groupCamPane = Gtk::manage(new Gtk::VPaned);

	_groupCamPane->pack1(*camWindow, true, true);	// allow shrinking
	_groupCamPane->pack2(*groupPane, true, false);	// no shrinking

	_horizPane.reset(new Gtk::HPaned);

	_horizPane->pack1(*_groupCamPane, true, false);	// no shrinking
	_horizPane->pack2(*xyView, true, true);			// allow shrinking

	// Retrieve the main container of the main window
	Gtk::Container* mainContainer = GlobalMainFrame().getMainContainer();
	mainContainer->add(*_horizPane);

	// Set some default values for the width and height
	_horizPane->set_position(500);
	_groupCamPane->set_position(350);

	// Connect the pane position trackers
	_posHPane.connect(_horizPane.get());
	_posGroupCamPane.connect(_groupCamPane);

	// Attempt to restore this layout's state
	restoreStateFromPath(RKEY_EMBEDDED_ROOT);

	mainContainer->show_all();

	// This is needed to fix a weirdness when re-parenting the entity inspector
	GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload
	// the position info from the Registry once again.
	GlobalEntityInspector().restoreSettings();

	// Reparent the notebook to our local pane (after the other widgets have been realised)
	GlobalGroupDialog().reparentNotebook(groupPane);

	// Hide the floating window again
	GlobalGroupDialog().hideDialogWindow();

	// Create the texture window
	Gtk::Frame* texWindow = Gtk::manage(new gtkutil::FramedWidget(
		*GlobalTextureBrowser().constructWindow(GlobalMainFrame().getTopLevelWindow())
	));

	// Add the Texture Browser page to the group dialog
	GlobalGroupDialog().addPage(
    	"textures",	// name
    	"Textures", // tab title
    	"icon_texture.png", // tab icon
    	*texWindow, // page widget
    	_("Texture Browser")
    );

	// Hide the camera toggle option for non-floating views
    GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", false);
	// Hide the console/texture browser toggles for non-floating/non-split views
	GlobalUIManager().getMenuManager().setVisibility("main/view/textureBrowser", false);
}

void EmbeddedLayout::deactivate()
{
	// Show the camera toggle option again
    GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", true);
	GlobalUIManager().getMenuManager().setVisibility("main/view/textureBrowser", true);

	// Remove all previously stored pane information
	GlobalRegistry().deleteXPath(RKEY_EMBEDDED_ROOT + "//pane");

	// Save pane info
	saveStateToPath(RKEY_EMBEDDED_ROOT);

	// Delete all active views
	GlobalXYWndManager().destroyViews();

	// Delete the CamWnd
	_camWnd = CamWndPtr();

	// Give the notebook back to the GroupDialog
	GlobalGroupDialog().reparentNotebookToSelf();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	GlobalGroupDialog().removePage("textures");
	GlobalTextureBrowser().destroyWindow();

	// Destroy the horizpane widget, so it gets removed from the main container
	_horizPane.reset();
}

void EmbeddedLayout::maximiseCameraSize()
{
	// Save the current state to the registry
	saveStateToPath(RKEY_EMBEDDED_TEMP_ROOT);

	// Maximise the camera
	_posHPane.applyMaxPosition();
	_posGroupCamPane.applyMaxPosition();
}

void EmbeddedLayout::restorePanePositions()
{
	// Restore state
	restoreStateFromPath(RKEY_EMBEDDED_TEMP_ROOT);

	// Remove all previously stored pane information
	GlobalRegistry().deleteXPath(RKEY_EMBEDDED_TEMP_ROOT);
}

void EmbeddedLayout::restoreStateFromPath(const std::string& path)
{
	// Now load the paned positions from the registry
	if (GlobalRegistry().keyExists(path + "/pane[@name='horizontal']"))
	{
		_posHPane.loadFromPath(path + "/pane[@name='horizontal']");
		_posHPane.applyPosition();
	}

	if (GlobalRegistry().keyExists(path + "/pane[@name='texcam']"))
	{
		_posGroupCamPane.loadFromPath(path + "/pane[@name='texcam']");
		_posGroupCamPane.applyPosition();
	}
}

void EmbeddedLayout::saveStateToPath(const std::string& path)
{
	GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
	_posHPane.saveToPath(path + "/pane[@name='horizontal']");

	GlobalRegistry().createKeyWithName(path, "pane", "texcam");
	_posGroupCamPane.saveToPath(path + "/pane[@name='texcam']");
}

void EmbeddedLayout::toggleFullscreenCameraView()
{
	if (GlobalRegistry().keyExists(RKEY_EMBEDDED_TEMP_ROOT))
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
EmbeddedLayoutPtr EmbeddedLayout::CreateInstance()
{
	return EmbeddedLayoutPtr(new EmbeddedLayout);
}

} // namespace ui
