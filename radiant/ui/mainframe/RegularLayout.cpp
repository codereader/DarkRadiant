#include "RegularLayout.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include "gtkutil/FramedWidget.h"
#include "gtkutil/Paned.h"

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

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

	GtkWindow* parent = GlobalMainFrame().getTopLevelWindow();

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd();
	 // greebo: The mainframe window acts as parent for the camwindow
	_camWnd->setContainer(parent);
	// Pack in the camera window
	GtkWidget* camWindow = gtkutil::FramedWidget(_camWnd->getWidget());

	// Allocate a new OrthoView and set its ViewType to XY
	XYWndPtr xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
    xyWnd->setViewType(XY);
    // Create a framed window out of the view's internal widget
    GtkWidget* xyView = gtkutil::FramedWidget(xyWnd->getWidget());

	// Create the texture window
	GtkWidget* texWindow = gtkutil::FramedWidget(
		GlobalTextureBrowser().constructWindow(parent)
	);

	// Now pack those widgets into the paned widgets
	gtkutil::Paned texCamPane(gtkutil::Paned::Vertical);

	// First, pack the texwindow and the camera
	texCamPane.setFirstChild(camWindow, true); // allow shrinking
	texCamPane.setSecondChild(texWindow, true); // allow shrinking

	_regular.texCamPane = texCamPane.getWidget();
    
    // Depending on the viewstyle, pack the xy left or right
	gtkutil::Paned horizPane(gtkutil::Paned::Horizontal);

    if (_regularLeft)
	{
		horizPane.setFirstChild(_regular.texCamPane, true); // allow shrinking
		horizPane.setSecondChild(xyView, true); // allow shrinking
    }
    else
	{
		// This is "regular", put the xyview to the left
		horizPane.setFirstChild(xyView, true); // allow shrinking
		horizPane.setSecondChild(_regular.texCamPane, true); // allow shrinking
    }

	_regular.horizPane = horizPane.getWidget();
    
	// Retrieve the main container of the main window
	GtkWidget* mainContainer = GlobalMainFrame().getMainContainer();
	gtk_container_add(GTK_CONTAINER(mainContainer), GTK_WIDGET(_regular.horizPane));

	// Set some default values for the width and height
	gtk_paned_set_position(GTK_PANED(_regular.horizPane), 500);
	gtk_paned_set_position(GTK_PANED(_regular.texCamPane), 350);

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

	gtk_widget_show_all(mainContainer);

	// Hide the camera toggle option for non-floating views
    GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", false);
	// Hide the console/texture browser toggles for non-floating/non-split views
	GlobalUIManager().getMenuManager().setVisibility("main/view/textureBrowser", false);	
}

void RegularLayout::deactivate() {
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
	gtk_widget_destroy(GTK_WIDGET(_regular.horizPane));
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
