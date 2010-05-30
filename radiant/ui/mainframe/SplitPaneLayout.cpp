#include "SplitPaneLayout.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include "gtkutil/FramedWidget.h"

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"

namespace ui {

	namespace
	{
		const std::string RKEY_SPLITPANE_ROOT = "user/ui/mainFrame/splitPane";
		const std::string RKEY_SPLITPANE_TEMP_ROOT = RKEY_SPLITPANE_ROOT + "/temp";

		const std::string RKEY_SPLITPANE_CAMPOS = RKEY_SPLITPANE_ROOT + "/cameraPosition";
	}

std::string SplitPaneLayout::getName()
{
	return SPLITPANE_LAYOUT_NAME;
}

void SplitPaneLayout::activate()
{
	constructLayout();
	constructMenus();
}

void SplitPaneLayout::constructLayout()
{
	_splitPane = SplitPaneView();

	_cameraPosition = getCameraPositionFromRegistry();

	GtkWindow* parent = GlobalMainFrame().getTopLevelWindow();

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd();
	 // greebo: The mainframe window acts as parent for the camwindow
	_camWnd->setContainer(parent);

	_camera = gtkutil::FramedWidget(_camWnd->getWidget());

	// Allocate the three ortho views
    XYWndPtr xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
    xyWnd->setViewType(XY);
	_orthoViews[XY] = gtkutil::FramedWidget(xyWnd->getWidget());
    
    XYWndPtr yzWnd = GlobalXYWnd().createEmbeddedOrthoView();
    yzWnd->setViewType(YZ);
    _orthoViews[YZ] = gtkutil::FramedWidget(yzWnd->getWidget());

    XYWndPtr xzWnd = GlobalXYWnd().createEmbeddedOrthoView();
    xzWnd->setViewType(XZ);
    _orthoViews[XZ] = gtkutil::FramedWidget(xzWnd->getWidget());

	// Distribute widgets among quadrants
	distributeWidgets();

	// Arrange the widgets into the paned views
	_splitPane.horizPane.setFirstChild(_splitPane.vertPane1.getWidget(), true);
	_splitPane.horizPane.setSecondChild(_splitPane.vertPane2.getWidget(), true);

	// Retrieve the main container of the main window
	GtkWidget* mainContainer = GlobalMainFrame().getMainContainer();
	gtk_container_add(GTK_CONTAINER(mainContainer), _splitPane.horizPane.getWidget());

	gtk_paned_set_position(GTK_PANED(_splitPane.horizPane.getWidget()), 200);
	gtk_paned_set_position(GTK_PANED(_splitPane.vertPane1.getWidget()), 200);
	gtk_paned_set_position(GTK_PANED(_splitPane.vertPane2.getWidget()), 400);

	_splitPane.posHPane.connect(_splitPane.horizPane.getWidget());
	_splitPane.posVPane1.connect(_splitPane.vertPane1.getWidget());
	_splitPane.posVPane2.connect(_splitPane.vertPane2.getWidget());
	
	// Attempt to restore this layout's state
	restoreStateFromPath(RKEY_SPLITPANE_ROOT);
	
    {      
		GtkWidget* textureBrowser = gtkutil::FramedWidget(
			GlobalTextureBrowser().constructWindow(parent)
		);

		// Add the Media Browser page
		GlobalGroupDialog().addPage(
	    	"textures",	// name
	    	"Textures", // tab title
	    	"icon_texture.png", // tab icon 
	    	GTK_WIDGET(textureBrowser), // page widget
	    	_("Texture Browser")
	    );
    }

	GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload 
	// the position info from the Registry once again.
	GlobalEntityInspector().restoreSettings();

	GlobalGroupDialog().hideDialogWindow();

	gtk_widget_show_all(mainContainer);
}

void SplitPaneLayout::constructMenus()
{
	// Hide the camera toggle option for non-floating views
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();
	menuManager.setVisibility("main/view/cameraview", false);

	// Add the commands for changing the camera position
	GlobalEventManager().addToggle("CameraPositionTopLeft", boost::bind(&SplitPaneLayout::setCameraTopLeft, this, _1));
	GlobalEventManager().addToggle("CameraPositionTopRight", boost::bind(&SplitPaneLayout::setCameraTopRight, this, _1));
	GlobalEventManager().addToggle("CameraPositionBottomLeft", boost::bind(&SplitPaneLayout::setCameraBottomLeft, this, _1));
	GlobalEventManager().addToggle("CameraPositionBottomRight", boost::bind(&SplitPaneLayout::setCameraBottomRight, this, _1));
	
	// Add the corresponding menu items
	menuManager.insert("main/view/camera", "cameraposition", 
					ui::menuFolder, _("Camera Position"), "", "");

	menuManager.add("main/view/cameraposition", "camtopleft", 
					ui::menuItem, _("Top Left"), "", "CameraPositionTopLeft");
	menuManager.add("main/view/cameraposition", "camtopright", 
					ui::menuItem, _("Top Right"), "", "CameraPositionTopRight");
	menuManager.add("main/view/cameraposition", "cambottomleft", 
					ui::menuItem, _("Bottom Left"), "", "CameraPositionBottomLeft");
	menuManager.add("main/view/cameraposition", "cambottomright", 
					ui::menuItem, _("Bottom Right"), "", "CameraPositionBottomRight");

	updateCameraPositionToggles();
}

void SplitPaneLayout::deconstructMenus()
{
	// Show the camera toggle option again
    GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", true);

	// Remove the camera position menu items
	GlobalUIManager().getMenuManager().remove("main/view/cameraposition");

	// Remove the camera position events
	GlobalEventManager().removeEvent("CameraPositionTopLeft");
	GlobalEventManager().removeEvent("CameraPositionTopRight");
	GlobalEventManager().removeEvent("CameraPositionBottomLeft");
	GlobalEventManager().removeEvent("CameraPositionBottomRight");
}

void SplitPaneLayout::deactivate()
{
	deconstructMenus();
	deconstructLayout();
}

void SplitPaneLayout::deconstructLayout()
{
	if (GlobalRegistry().keyExists(RKEY_SPLITPANE_TEMP_ROOT))
	{
		// We're maximised, restore the size first
		restorePanePositions();
	}

	// Save camera position
	saveCameraPositionToRegistry();

	// Remove all previously saved pane information 
	GlobalRegistry().deleteXPath(RKEY_SPLITPANE_ROOT + "//pane");

	// Save the pane info
	saveStateToPath(RKEY_SPLITPANE_ROOT);

	// Delete all active views
	GlobalXYWndManager().destroyViews();

	// Delete the CamWnd
	_camWnd = CamWndPtr();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");
	GlobalTextureBrowser().destroyWindow();

	// Destroy the widget, so it gets removed from the main container
	gtk_widget_destroy(_splitPane.horizPane.getWidget());
}

void SplitPaneLayout::maximiseCameraSize()
{
	// Save the current state to the registry
	saveStateToPath(RKEY_SPLITPANE_TEMP_ROOT);

	// Maximise the pane positions
	_splitPane.posHPane.applyMaxPosition();
	_splitPane.posVPane1.applyMaxPosition();
}

void SplitPaneLayout::restorePanePositions()
{
	// Restore state
	restoreStateFromPath(RKEY_SPLITPANE_TEMP_ROOT);

	// Remove all previously stored pane information
	GlobalRegistry().deleteXPath(RKEY_SPLITPANE_TEMP_ROOT);
}

void SplitPaneLayout::restoreStateFromPath(const std::string& path)
{
	if (GlobalRegistry().keyExists(path + "/pane[@name='horizontal']"))
	{
		_splitPane.posHPane.loadFromPath(path + "/pane[@name='horizontal']");
		_splitPane.posHPane.applyPosition();
	}
	
	if (GlobalRegistry().keyExists(path + "/pane[@name='vertical1']"))
	{
		_splitPane.posVPane1.loadFromPath(path + "/pane[@name='vertical1']");
		_splitPane.posVPane1.applyPosition();
	}
	
	if (GlobalRegistry().keyExists(path + "/pane[@name='vertical2']"))
	{
		_splitPane.posVPane2.loadFromPath(path + "/pane[@name='vertical2']");
		_splitPane.posVPane2.applyPosition();
	}
}

void SplitPaneLayout::saveStateToPath(const std::string& path)
{
	GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
	_splitPane.posHPane.saveToPath(path + "/pane[@name='horizontal']");
	
	GlobalRegistry().createKeyWithName(path, "pane", "vertical1");
	_splitPane.posVPane1.saveToPath(path + "/pane[@name='vertical1']");

	GlobalRegistry().createKeyWithName(path, "pane", "vertical2");
	_splitPane.posVPane2.saveToPath(path + "/pane[@name='vertical2']");
}

void SplitPaneLayout::toggleFullscreenCameraView()
{
	if (GlobalRegistry().keyExists(RKEY_SPLITPANE_TEMP_ROOT))
	{
		restorePanePositions();
	}
	else
	{
		// No saved info found in registry, maximise cam
		maximiseCameraSize();
	}
}

SplitPaneLayout::Position SplitPaneLayout::getCameraPositionFromRegistry()
{
	int value = GlobalRegistry().getInt(RKEY_SPLITPANE_CAMPOS);

	if (value < QuadrantTopLeft || value > QuadrantBottomRight)
	{
		value = static_cast<int>(QuadrantTopLeft);
	}

	return static_cast<Position>(value);
}

void SplitPaneLayout::saveCameraPositionToRegistry()
{
	GlobalRegistry().setInt(RKEY_SPLITPANE_CAMPOS, static_cast<int>(_cameraPosition));
}

void SplitPaneLayout::setCameraTopLeft(bool newState)
{
	if (_cameraPosition == QuadrantTopLeft && newState) return; // nop

	// Only react to "activate" events or same type
	if (newState || _cameraPosition == QuadrantTopLeft)
	{
		_cameraPosition = QuadrantTopLeft;
		
		deconstructLayout();
		constructLayout();

		updateCameraPositionToggles();
	}
}

void SplitPaneLayout::setCameraTopRight(bool newState)
{
	if (_cameraPosition == QuadrantTopRight && newState) return; // nop

	// Only react to "activate" events
	if (newState || _cameraPosition == QuadrantTopRight)
	{
		_cameraPosition = QuadrantTopRight;
		
		deconstructLayout();
		constructLayout();

		updateCameraPositionToggles();
	}
}

void SplitPaneLayout::setCameraBottomLeft(bool newState)
{
	if (_cameraPosition == QuadrantBottomLeft && newState) return; // nop

	// Only react to "activate" events
	if (newState || _cameraPosition == QuadrantBottomLeft)
	{
		_cameraPosition = QuadrantBottomLeft;

		deconstructLayout();
		constructLayout();

		updateCameraPositionToggles();
	}
}

void SplitPaneLayout::setCameraBottomRight(bool newState)
{
	if (_cameraPosition == QuadrantBottomRight && newState) return; // nop

	// Only react to "activate" events
	if (newState || _cameraPosition == QuadrantBottomRight)
	{
		_cameraPosition = QuadrantBottomRight;
		
		deconstructLayout();
		constructLayout();

		updateCameraPositionToggles();
	}
}

void SplitPaneLayout::updateCameraPositionToggles()
{
	// Update toggle state
	GlobalEventManager().setToggled("CameraPositionTopLeft", _cameraPosition == QuadrantTopLeft);
	GlobalEventManager().setToggled("CameraPositionTopRight", _cameraPosition == QuadrantTopRight);
	GlobalEventManager().setToggled("CameraPositionBottomLeft", _cameraPosition == QuadrantBottomLeft);
	GlobalEventManager().setToggled("CameraPositionBottomRight", _cameraPosition == QuadrantBottomRight);
}

void SplitPaneLayout::distributeWidgets()
{
	// Clear mapping
	_quadrants[QuadrantTopLeft] = NULL;
	_quadrants[QuadrantTopRight] = NULL;
	_quadrants[QuadrantBottomLeft] = NULL;
	_quadrants[QuadrantBottomRight] = NULL;

	// Set camera
	_quadrants[_cameraPosition] = _camera;

	// Distribute the orthoviews in the gaps
	for (OrthoViewMap::const_iterator ortho = _orthoViews.begin(); ortho != _orthoViews.end(); ++ortho)
	{
		for (WidgetMap::iterator i = _quadrants.begin(); i != _quadrants.end(); ++i)
		{
			if (i->second == NULL) 
			{
				i->second = ortho->second;
				break;
			}
		}
	}

	_splitPane.vertPane1.setFirstChild(_quadrants[QuadrantTopLeft], true); // allow shrinking
	_splitPane.vertPane1.setSecondChild(_quadrants[QuadrantBottomLeft], true); // allow shrinking

	_splitPane.vertPane2.setFirstChild(_quadrants[QuadrantTopRight], true); // allow shrinking
	_splitPane.vertPane2.setSecondChild(_quadrants[QuadrantBottomRight], true); // allow shrinking
}

// The creation function, needed by the mainframe layout manager
SplitPaneLayoutPtr SplitPaneLayout::CreateInstance() {
	return SplitPaneLayoutPtr(new SplitPaneLayout);
}

} // namespace ui
