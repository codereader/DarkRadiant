#include "FloatingLayout.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ientityinspector.h"

#include "registry/registry.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

namespace ui {

	namespace {
		const std::string RKEY_CAMERA_ROOT = "user/ui/camera";
		const std::string RKEY_CAMERA_WINDOW_STATE = RKEY_CAMERA_ROOT + "/window";
		const std::string RKEY_FLOATING_ROOT = "user/ui/mainFrame/floating";
		const std::string RKEY_GROUPDIALOG_VISIBLE = RKEY_FLOATING_ROOT + "/groupDialogVisible";
	}

std::string FloatingLayout::getName() {
	return FLOATING_LAYOUT_NAME;
}

void FloatingLayout::activate() {
 	// Get the floating window with the CamWnd packed into it
	_floatingCamWnd = GlobalCamera().createFloatingWindow();
	GlobalEventManager().connectAccelGroup(_floatingCamWnd.get());

	// Restore the window position from the registry if possible
	if (!GlobalRegistry().findXPath(RKEY_CAMERA_WINDOW_STATE).empty())
	{
		_camWndPosition.loadFromPath(RKEY_CAMERA_WINDOW_STATE);
		_camWndPosition.connect(_floatingCamWnd.get());
	}

	_floatingCamWnd->show();

	// Connect up the toggle camera event
	IEventPtr ev = GlobalEventManager().findEvent("ToggleCamera");
	if (!ev->empty()) {
		ev->connectWidget(_floatingCamWnd.get());
		ev->updateWidgets();
	}
	else {
		rError() << "Could not connect ToggleCamera event\n";
	}

	Glib::RefPtr<Gtk::Window> groupDialog(GlobalGroupDialog().getDialogWindow());

	Gtk::Widget* page = Gtk::manage(new gtkutil::FramedWidget(
		*GlobalTextureBrowser().constructWindow(groupDialog)
	));

	// Add the Texture Browser page to the group dialog
	GlobalGroupDialog().addPage(
    	"textures",	// name
    	"Textures", // tab title
    	"icon_texture.png", // tab icon
    	*page, // page widget
    	_("Texture Browser")
    );

	if (registry::getValue<bool>(RKEY_GROUPDIALOG_VISIBLE))
	{
		GlobalGroupDialog().showDialogWindow();
	}

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload
	// the position info from the Registry once again.
	GlobalEntityInspector().restoreSettings();

	// Restore any floating XYViews that were active before
	// This will create a default view if no saved info is found
	GlobalXYWnd().restoreState();
}

void FloatingLayout::deactivate()
{
	// Save the current XYViews to the registry
	GlobalXYWnd().saveState();

	// Delete all active views
	GlobalXYWndManager().destroyViews();

	// Save groupdialog state
    registry::setValue(RKEY_GROUPDIALOG_VISIBLE,
                       GlobalGroupDialog().getDialogWindow()->is_visible());

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");
	GlobalTextureBrowser().destroyWindow();

	// Destroy the camera window
	if (_floatingCamWnd != NULL)
	{
		if (_floatingCamWnd->isFullscreen())
		{
			_floatingCamWnd->setFullscreen(false);
		}

		// Save camwnd state
		_camWndPosition.saveToPath(RKEY_CAMERA_WINDOW_STATE);

		IEventPtr ev = GlobalEventManager().findEvent("ToggleCamera");
		if (!ev->empty()) {
			ev->disconnectWidget(_floatingCamWnd.get());
		}
		else {
			rError() << "Could not disconnect ToggleCamera event\n";
		}

		// Release the object
		_floatingCamWnd = gtkutil::PersistentTransientWindowPtr();
	}
}

void FloatingLayout::toggleFullscreenCameraView()
{
	_floatingCamWnd->toggleFullscreen();
}

// The creation function, needed by the mainframe layout manager
FloatingLayoutPtr FloatingLayout::CreateInstance() {
	return FloatingLayoutPtr(new FloatingLayout);
}

} // namespace ui
