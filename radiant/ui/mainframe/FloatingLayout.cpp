#include "FloatingLayout.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"

#include "camera/GlobalCamera.h"
#include "ui/einspector/EntityInspector.h"
#include "ui/texturebrowser/TextureBrowser.h"

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
	_floatingCamWnd = GlobalCamera().getFloatingWindow();
	GlobalEventManager().connectAccelGroup(GTK_WINDOW(_floatingCamWnd->getWindow()));

	// Restore the window position from the registry if possible
	xml::NodeList windowStateList = 
		GlobalRegistry().findXPath(RKEY_CAMERA_WINDOW_STATE);
	
	if (!windowStateList.empty()) {
		_camWndPosition.loadFromNode(windowStateList[0]);
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
	GlobalEventManager().addCommand("ToggleCameraFullScreen", 
		MemberCaller<gtkutil::TransientWindow, 
					 &gtkutil::TransientWindow::toggleFullscreen>(*_floatingCamWnd)
	);

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
	ui::EntityInspector::getInstance().restoreSettings();
}

void FloatingLayout::deactivate() {
	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");
	GlobalTextureBrowser().destroyWindow();

	// Destroy the camera window
	if (_floatingCamWnd != NULL) {
		// Save camwnd state
		// Delete all the current window states from the registry  
		GlobalRegistry().deleteXPath(RKEY_CAMERA_WINDOW_STATE);
		
		// Create a new node
		xml::Node node(GlobalRegistry().createKey(RKEY_CAMERA_WINDOW_STATE));
		
		_camWndPosition.saveToNode(node);

		// Release the object
		_floatingCamWnd = gtkutil::PersistentTransientWindowPtr();

		// De-register commands
		GlobalEventManager().removeEvent("ToggleCameraFullScreen");
	}
}

// The creation function, needed by the mainframe layout manager
FloatingLayoutPtr FloatingLayout::CreateInstance() {
	return FloatingLayoutPtr(new FloatingLayout);
}

} // namespace ui
