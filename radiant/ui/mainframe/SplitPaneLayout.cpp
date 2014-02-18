#include "SplitPaneLayout.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include "registry/registry.h"
#include "gtkutil/FramedWidget.h"

#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"

#include <boost/bind.hpp>
#include <gtkmm/paned.h>

namespace ui {

	namespace
	{
		const std::string RKEY_SPLITPANE_ROOT = "user/ui/mainFrame/splitPane";
		const std::string RKEY_SPLITPANE_TEMP_ROOT = RKEY_SPLITPANE_ROOT + "/temp";

		const std::string RKEY_SPLITPANE_CAMPOS = RKEY_SPLITPANE_ROOT + "/cameraPosition";
		const std::string RKEY_SPLITPANE_VIEWTYPES = RKEY_SPLITPANE_ROOT + "/viewTypes";
	}

SplitPaneLayout::SplitPaneLayout()
{
	clearQuadrantInfo();
}

void SplitPaneLayout::clearQuadrantInfo()
{
	_quadrants[QuadrantTopLeft] = QuadrantInfo();
	_quadrants[QuadrantTopRight] = QuadrantInfo();
	_quadrants[QuadrantBottomLeft] = QuadrantInfo();
	_quadrants[QuadrantBottomRight] = QuadrantInfo();
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

	const Glib::RefPtr<Gtk::Window>& parent = GlobalMainFrame().getTopLevelWindow();

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd();
	 // greebo: The mainframe window acts as parent for the camwindow
	_camWnd->setContainer(parent);

	_camera = Gtk::manage(new gtkutil::FramedWidget(*_camWnd->getWidget()));

	// Allocate paned widgets
	_splitPane.horizPane.reset(new Gtk::HPaned);
	_splitPane.vertPane1 = Gtk::manage(new Gtk::VPaned);
	_splitPane.vertPane2 = Gtk::manage(new Gtk::VPaned);

	// Arrange the widgets into the paned views
	_splitPane.horizPane->pack1(*_splitPane.vertPane1, true, true);
	_splitPane.horizPane->pack2(*_splitPane.vertPane2, true, true);

	// Retrieve the main container of the main window
	Gtk::Container* mainContainer = GlobalMainFrame().getMainContainer();
	mainContainer->add(*_splitPane.horizPane);

	_splitPane.horizPane->set_position(200);
	_splitPane.vertPane1->set_position(200);
	_splitPane.vertPane2->set_position(400);

	_splitPane.posHPane.connect(_splitPane.horizPane.get());
	_splitPane.posVPane1.connect(_splitPane.vertPane1);
	_splitPane.posVPane2.connect(_splitPane.vertPane2);

	// Attempt to restore this layout's state, this will also construct the orthoviews
	restoreStateFromPath(RKEY_SPLITPANE_ROOT);

	// Distribute widgets among quadrants
	distributeWidgets();

    {
		Gtk::Frame* textureBrowser = Gtk::manage(new gtkutil::FramedWidget(
			*GlobalTextureBrowser().constructWindow(parent)
		));

		// Add the Media Browser page
		GlobalGroupDialog().addPage(
	    	"textures",	// name
	    	"Textures", // tab title
	    	"icon_texture.png", // tab icon
	    	*textureBrowser, // page widget
	    	_("Texture Browser")
	    );
    }

	GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload
	// the position info from the Registry once again.
	GlobalEntityInspector().restoreSettings();

	GlobalGroupDialog().hideDialogWindow();

	mainContainer->show_all();
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

	// Reset quadrant information
	clearQuadrantInfo();

	// Delete all active views
	GlobalXYWndManager().destroyViews();

	// Delete the CamWnd
	_camWnd = CamWndPtr();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");
	GlobalTextureBrowser().destroyWindow();

	// Destroy the widgets, so it gets removed from the main container
	_splitPane = SplitPaneView();
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

	int topLeft = string::convert<int>(GlobalRegistry().getAttribute(RKEY_SPLITPANE_VIEWTYPES, "topleft"), -1);
	int topRight = string::convert<int>(GlobalRegistry().getAttribute(RKEY_SPLITPANE_VIEWTYPES, "topright"), XY);
	int bottomLeft = string::convert<int>(GlobalRegistry().getAttribute(RKEY_SPLITPANE_VIEWTYPES, "bottomleft"), YZ);
	int bottomRight = string::convert<int>(GlobalRegistry().getAttribute(RKEY_SPLITPANE_VIEWTYPES, "bottomright"), XZ);

	// Load mapping, but leave widget pointer NULL
	_quadrants[QuadrantTopLeft] = QuadrantInfo();
	_quadrants[QuadrantTopLeft].isCamera = (topLeft == -1);

	if (!_quadrants[QuadrantTopLeft].isCamera)
	{
		_quadrants[QuadrantTopLeft].xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
		_quadrants[QuadrantTopLeft].xyWnd->setViewType(static_cast<EViewType>(topLeft));
	}

	_quadrants[QuadrantTopRight] = QuadrantInfo();
	_quadrants[QuadrantTopRight].isCamera = (topRight == -1);

	if (!_quadrants[QuadrantTopRight].isCamera)
	{
		_quadrants[QuadrantTopRight].xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
		_quadrants[QuadrantTopRight].xyWnd->setViewType(static_cast<EViewType>(topRight));
	}

	_quadrants[QuadrantBottomLeft] = QuadrantInfo();
	_quadrants[QuadrantBottomLeft].isCamera = (bottomLeft == -1);

	if (!_quadrants[QuadrantBottomLeft].isCamera)
	{
		_quadrants[QuadrantBottomLeft].xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
		_quadrants[QuadrantBottomLeft].xyWnd->setViewType(static_cast<EViewType>(bottomLeft));
	}

	_quadrants[QuadrantBottomRight] = QuadrantInfo();
	_quadrants[QuadrantBottomRight].isCamera = (bottomRight == -1);

	if (!_quadrants[QuadrantBottomRight].isCamera)
	{
		_quadrants[QuadrantBottomRight].xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
		_quadrants[QuadrantBottomRight].xyWnd->setViewType(static_cast<EViewType>(bottomRight));
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

	GlobalRegistry().deleteXPath(RKEY_SPLITPANE_VIEWTYPES);
	xml::Node node = GlobalRegistry().createKey(RKEY_SPLITPANE_VIEWTYPES);

	// Camera is assigned -1 as viewtype
	int topLeft = _quadrants[QuadrantTopLeft].xyWnd != NULL ? _quadrants[QuadrantTopLeft].xyWnd->getViewType() : -1;
	int topRight = _quadrants[QuadrantTopRight].xyWnd != NULL ? _quadrants[QuadrantTopRight].xyWnd->getViewType() : -1;
	int bottomLeft = _quadrants[QuadrantBottomLeft].xyWnd != NULL ? _quadrants[QuadrantBottomLeft].xyWnd->getViewType() : -1;
	int bottomRight = _quadrants[QuadrantBottomRight].xyWnd != NULL ? _quadrants[QuadrantBottomRight].xyWnd->getViewType() : -1;

	node.setAttributeValue("topleft", string::to_string(topLeft));
	node.setAttributeValue("topright", string::to_string(topRight));
	node.setAttributeValue("bottomleft", string::to_string(bottomLeft));
	node.setAttributeValue("bottomright", string::to_string(bottomRight));
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
	int value = registry::getValue<int>(RKEY_SPLITPANE_CAMPOS);

	if (value < QuadrantTopLeft || value > QuadrantBottomRight)
	{
		value = static_cast<int>(QuadrantTopLeft);
	}

	return static_cast<Position>(value);
}

void SplitPaneLayout::saveCameraPositionToRegistry()
{
	registry::setValue(RKEY_SPLITPANE_CAMPOS, static_cast<int>(_cameraPosition));
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
	// Set camera position afresh
	_quadrants[_cameraPosition].widget = _camera;
	_quadrants[_cameraPosition].isCamera = true;

	for (WidgetMap::iterator i = _quadrants.begin(); i != _quadrants.end(); ++i)
	{
		if (i->second.widget == NULL)
		{
			// Missing widget, this must be an orthoview
			i->second.isCamera = false;

			// Ensure we have an XYWnd instance at hand
			if (i->second.xyWnd == NULL)
			{
				i->second.xyWnd = GlobalXYWnd().createEmbeddedOrthoView();

				// FIXME: This could be the third complementing viewtype
				i->second.xyWnd->setViewType(XY);
			}

			// Frame the widget to make it ready for packing
			i->second.widget = Gtk::manage(new gtkutil::FramedWidget(*i->second.xyWnd->getWidget()));
		}
	}

	_splitPane.vertPane1->pack1(*_quadrants[QuadrantTopLeft].widget, true, true); // allow shrinking
	_splitPane.vertPane1->pack2(*_quadrants[QuadrantBottomLeft].widget, true, true); // allow shrinking

	_splitPane.vertPane2->pack1(*_quadrants[QuadrantTopRight].widget, true, true); // allow shrinking
	_splitPane.vertPane2->pack2(*_quadrants[QuadrantBottomRight].widget, true, true); // allow shrinking
}

// The creation function, needed by the mainframe layout manager
SplitPaneLayoutPtr SplitPaneLayout::CreateInstance() {
	return SplitPaneLayoutPtr(new SplitPaneLayout);
}

} // namespace ui
