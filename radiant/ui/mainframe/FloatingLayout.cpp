#include "FloatingLayout.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ientityinspector.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"
#include <boost/bind.hpp>

namespace ui {

	namespace {
		const std::string RKEY_CAMERA_ROOT = "user/ui/camera"; 
		const std::string RKEY_CAMERA_WINDOW_STATE = RKEY_CAMERA_ROOT + "/window";
	}

std::string FloatingLayout::getName() {
	return FLOATING_LAYOUT_NAME;
}

void FloatingLayout::activate() {
 	// Get the floating window with the CamWnd packed into it
	_floatingCamWnd = GlobalCamera().createFloatingWindow();
	GlobalEventManager().connectAccelGroup(GTK_WINDOW(_floatingCamWnd->getWindow()));

	// Restore the window position from the registry if possible
	if (!GlobalRegistry().findXPath(RKEY_CAMERA_WINDOW_STATE).empty())
	{
		_camWndPosition.loadFromPath(RKEY_CAMERA_WINDOW_STATE);
		_camWndPosition.connect(
			GTK_WINDOW(_floatingCamWnd->getWindow())
		);
	}
	  
	_floatingCamWnd->show();

	// Connect up the toggle camera event
	IEventPtr ev = GlobalEventManager().findEvent("ToggleCamera");
	if (!ev->empty()) {
		ev->connectWidget(_floatingCamWnd->getWindow());
		ev->updateWidgets();
	}
	else {
		globalErrorStream() << "Could not connect ToggleCamera event\n";
	}

	// Add the toggle max/min command for floating windows
	GlobalCommandSystem().addCommand("ToggleCameraFullScreen", 
		boost::bind(&FloatingLayout::toggleCameraFullScreen, this, _1)
	);

	GlobalEventManager().addCommand("ToggleCameraFullScreen", "ToggleCameraFullScreen");

	GtkWidget* page = gtkutil::FramedWidget(
		GlobalTextureBrowser().constructWindow(GTK_WINDOW(GlobalGroupDialog().getDialogWindow()))
	);

	// Add the Texture Browser page to the group dialog
	GlobalGroupDialog().addPage(
    	"textures",	// name
    	"Textures", // tab title
    	"icon_texture.png", // tab icon 
    	GTK_WIDGET(page), // page widget
    	"Texture Browser"
    );

	GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload 
	// the position info from the Registry once again.
	GlobalEntityInspector().restoreSettings();

	// Restore any floating XYViews that were active before
	// This will create a default view if no saved info is found
	GlobalXYWnd().restoreState();
}

void FloatingLayout::deactivate() {
	// Save the current XYViews to the registry
	GlobalXYWnd().saveState();

	// Delete all active views
	GlobalXYWnd().destroyViews();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");
	GlobalTextureBrowser().destroyWindow();

	// Destroy the camera window
	if (_floatingCamWnd != NULL)
	{
		// Save camwnd state
		_camWndPosition.saveToPath(RKEY_CAMERA_WINDOW_STATE);

		// De-register commands
		GlobalEventManager().removeEvent("ToggleCameraFullScreen");
		GlobalCommandSystem().removeCommand("ToggleCameraFullScreen");

		IEventPtr ev = GlobalEventManager().findEvent("ToggleCamera");
		if (!ev->empty()) {
			ev->disconnectWidget(_floatingCamWnd->getWindow());
		}
		else {
			globalErrorStream() << "Could not disconnect ToggleCamera event\n";
		}

		// Release the object
		_floatingCamWnd = gtkutil::PersistentTransientWindowPtr();
	}
}

void FloatingLayout::toggleCameraFullScreen(const cmd::ArgumentList& args) {
	_floatingCamWnd->toggleFullscreen();
}

// The creation function, needed by the mainframe layout manager
FloatingLayoutPtr FloatingLayout::CreateInstance() {
	return FloatingLayoutPtr(new FloatingLayout);
}

} // namespace ui
