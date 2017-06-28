#include "FloatingLayout.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ientityinspector.h"

#include "registry/registry.h"

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"

namespace ui
{

namespace
{
	const std::string RKEY_CAMERA_ROOT = "user/ui/camera";
	const std::string RKEY_CAMERA_WINDOW_STATE = RKEY_CAMERA_ROOT + "/window";
	const std::string RKEY_FLOATING_ROOT = "user/ui/mainFrame/floating";
	const std::string RKEY_GROUPDIALOG_VISIBLE = RKEY_FLOATING_ROOT + "/groupDialogVisible";
}

std::string FloatingLayout::getName()
{
	return FLOATING_LAYOUT_NAME;
}

void FloatingLayout::activate()
{
	wxFrame* topLevelWindow = GlobalMainFrame().getWxTopLevelWindow();

	_floatingCamWnd = GlobalCamera().createFloatingWindow();
	
	// Connect up the toggle camera event
	IEventPtr ev = GlobalEventManager().findEvent("ToggleCamera");

	if (!ev->empty())
	{
		ev->connectTopLevelWindow(_floatingCamWnd.get());
		ev->updateWidgets();
	}
	else
	{
		rError() << "Could not connect ToggleCamera event" <<  std::endl;
	}

	// Add a new texture browser to the group dialog pages
	wxWindow* textureBrowser = new TextureBrowser(topLevelWindow);

	// Texture Page
	{
		IGroupDialog::PagePtr page(new IGroupDialog::Page);

		page->name = "textures";
		page->windowLabel = _("Texture Browser");
		page->page = textureBrowser;
		page->tabIcon = "icon_texture.png";
		page->tabLabel = _("Textures");
		page->position = IGroupDialog::Page::Position::TextureBrowser;

		GlobalGroupDialog().addPage(page);
	}

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

	// Show the camera and restore its position. Curiously enough,
	// calling Show() earlier would not restore its position correctly
	// but move the camera off a dozen pixels or so
	_floatingCamWnd->Show();
}

void FloatingLayout::deactivate()
{
	// Save the current XYViews to the registry
	GlobalXYWnd().saveState();

	// Delete all active views
	GlobalXYWndManager().destroyViews();

	// Save groupdialog state
    /*registry::setValue(RKEY_GROUPDIALOG_VISIBLE,
		GlobalGroupDialog().getWxDialogWindow()->IsShownOnScreen());*/

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");

	// Destroy the camera window
	if (_floatingCamWnd != NULL)
	{
		if (_floatingCamWnd->IsFullScreen())
		{
			_floatingCamWnd->ShowFullScreen(false);
		}

		IEventPtr ev = GlobalEventManager().findEvent("ToggleCamera");

		if (!ev->empty())
		{
			ev->disconnectTopLevelWindow(_floatingCamWnd.get());
		}
		else
		{
			rError() << "Could not disconnect ToggleCamera event" << std::endl;
		}

		// Release the object
		_floatingCamWnd.reset();
	}
}

void FloatingLayout::toggleFullscreenCameraView()
{
	_floatingCamWnd->ShowFullScreen(!_floatingCamWnd->IsFullScreen());
}

void FloatingLayout::restoreStateFromRegistry()
{
	// nothing yet
}

// The creation function, needed by the mainframe layout manager
FloatingLayoutPtr FloatingLayout::CreateInstance()
{
	return std::make_shared<FloatingLayout>();
}

} // namespace ui
