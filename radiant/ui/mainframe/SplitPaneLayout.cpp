#include "SplitPaneLayout.h"

#include "i18n.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"
#include "ientityinspector.h"

#include "registry/registry.h"

#include "camera/CamWnd.h"
#include "camera/GlobalCamera.h"
#include "ui/texturebrowser/TextureBrowser.h"

#include <wx/splitter.h>
#include <wx/sizer.h>

#include <functional>

namespace ui
{

namespace
{
	const std::string RKEY_SPLITPANE_ROOT = "user/ui/mainFrame/splitPane";
	const std::string RKEY_SPLITPANE_TEMP_ROOT = RKEY_SPLITPANE_ROOT + "/temp";

	const std::string RKEY_SPLITPANE_CAMPOS = RKEY_SPLITPANE_ROOT + "/cameraPosition";
	const std::string RKEY_SPLITPANE_VIEWTYPES = RKEY_SPLITPANE_ROOT + "/viewTypes";

    // Ensures enum boundaries before casting
    inline EViewType intToViewType(int type)
    {
        return type < YZ || type > XY ? XY : static_cast<EViewType>(type);
    }
}

SplitPaneLayout::SplitPaneLayout()
{
	clearQuadrantInfo();

	_splitPane.clear();
}

void SplitPaneLayout::clearQuadrantInfo()
{
	_quadrants[QuadrantTopLeft] = Quadrant();
	_quadrants[QuadrantTopRight] = Quadrant();
	_quadrants[QuadrantBottomLeft] = Quadrant();
	_quadrants[QuadrantBottomRight] = Quadrant();
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
	_splitPane.clear();

    wxFrame* topLevelParent = GlobalMainFrame().getWxTopLevelWindow();

	// Main splitter
	_splitPane.horizPane = new wxSplitterWindow(topLevelParent, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, 
		wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "SplitPaneHorizPane");

	GlobalMainFrame().getWxMainContainer()->Add(_splitPane.horizPane, 1, wxEXPAND);

	// Two sub-splitters
	_splitPane.vertPane1  = new wxSplitterWindow(_splitPane.horizPane, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, 
		wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "SplitPaneVertPane1");

	_splitPane.vertPane2  = new wxSplitterWindow(_splitPane.horizPane, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, 
		wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "SplitPaneVertPane2");

    _splitPane.vertPane1->SetMinimumPaneSize(1); // disallow unsplitting
    _splitPane.vertPane2->SetMinimumPaneSize(1); // disallow unsplitting

	_splitPane.horizPane->SplitVertically(_splitPane.vertPane1, _splitPane.vertPane2);

	_splitPane.posHPane.connect(_splitPane.horizPane);
	_splitPane.posVPane1.connect(_splitPane.vertPane1);
	_splitPane.posVPane2.connect(_splitPane.vertPane2);

    // Create the views and place them into the splitters
    createViews();

	// Attempt to restore this layout's state, this will also construct the orthoviews
	restoreStateFromPath(RKEY_SPLITPANE_ROOT);

	// Add a new texture browser to the group dialog pages
    wxWindow* textureBrowser = new TextureBrowser(topLevelParent);

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
	
	GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload
	// the position info from the Registry once again.
	GlobalEntityInspector().restoreSettings();

	GlobalGroupDialog().hideDialogWindow();

	topLevelParent->Layout();
}

void SplitPaneLayout::constructMenus()
{
	// Hide the camera toggle option for non-floating views
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();
	menuManager.setVisibility("main/view/cameraview", false);

	// Add the commands for changing the camera position
	GlobalEventManager().addToggle("CameraPositionTopLeft", std::bind(&SplitPaneLayout::setCameraTopLeft, this, std::placeholders::_1));
	GlobalEventManager().addToggle("CameraPositionTopRight", std::bind(&SplitPaneLayout::setCameraTopRight, this, std::placeholders::_1));
	GlobalEventManager().addToggle("CameraPositionBottomLeft", std::bind(&SplitPaneLayout::setCameraBottomLeft, this, std::placeholders::_1));
	GlobalEventManager().addToggle("CameraPositionBottomRight", std::bind(&SplitPaneLayout::setCameraBottomRight, this, std::placeholders::_1));

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

	_splitPane.posHPane.disconnect();
	_splitPane.posVPane1.disconnect();
	_splitPane.posVPane2.disconnect();

	// Reset quadrant information
	clearQuadrantInfo();

	// Delete all active views
	GlobalXYWndManager().destroyViews();

	// Delete the CamWnd
	_camWnd.reset();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");

	// Destroy the widgets, so it gets removed from the main container
	delete _splitPane.horizPane;
	
	_splitPane.clear();
}

void SplitPaneLayout::maximiseCameraSize()
{
	// Save the current state to the registry
	saveStateToPath(RKEY_SPLITPANE_TEMP_ROOT);

	// Maximise the pane positions
	_splitPane.horizPane->SetSashPosition(200000);
	_splitPane.vertPane1->SetSashPosition(200000);
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
    // Trigger a proper resize event before setting the sash position
    GlobalMainFrame().getWxTopLevelWindow()->SendSizeEvent();
    wxTheApp->Yield();

    if (GlobalRegistry().keyExists(path + "/pane[@name='horizontal']"))
    {
        _splitPane.posHPane.loadFromPath(path + "/pane[@name='horizontal']");
    }

    if (GlobalRegistry().keyExists(path + "/pane[@name='vertical1']"))
    {
        _splitPane.posVPane1.loadFromPath(path + "/pane[@name='vertical1']");
    }

    if (GlobalRegistry().keyExists(path + "/pane[@name='vertical2']"))
    {
        _splitPane.posVPane2.loadFromPath(path + "/pane[@name='vertical2']");
    }

    // Restore view types
    int topLeft = string::convert<int>(GlobalRegistry().getAttribute(RKEY_SPLITPANE_VIEWTYPES, "topleft"), -1);
    int topRight = string::convert<int>(GlobalRegistry().getAttribute(RKEY_SPLITPANE_VIEWTYPES, "topright"), XY);
    int bottomLeft = string::convert<int>(GlobalRegistry().getAttribute(RKEY_SPLITPANE_VIEWTYPES, "bottomleft"), YZ);
    int bottomRight = string::convert<int>(GlobalRegistry().getAttribute(RKEY_SPLITPANE_VIEWTYPES, "bottomright"), XZ);

    if (_quadrants[QuadrantTopLeft].xyWnd)
    {
        _quadrants[QuadrantTopLeft].xyWnd->setViewType(intToViewType(topLeft));
    }

    if (_quadrants[QuadrantTopRight].xyWnd)
    {
        _quadrants[QuadrantTopRight].xyWnd->setViewType(intToViewType(topRight));
    }

    if (_quadrants[QuadrantBottomLeft].xyWnd)
    {
        _quadrants[QuadrantBottomLeft].xyWnd->setViewType(intToViewType(bottomLeft));
    }

    if (_quadrants[QuadrantBottomRight].xyWnd)
    {
        _quadrants[QuadrantBottomRight].xyWnd->setViewType(intToViewType(bottomRight));
    }
}

void SplitPaneLayout::createViews()
{
    // We need to know the camera position first
    _cameraPosition = getCameraPositionFromRegistry();

	// Initialise mapping and create the views
    for (int i = QuadrantTopLeft; i <= QuadrantBottomRight; ++i)
    {
        Position pos = static_cast<Position>(i);

        Quadrant quadrant;

        quadrant.type = (_cameraPosition == pos) ? Quadrant::Camera : Quadrant::OrthoView;

        wxWindow* parent = (pos == QuadrantTopLeft || pos == QuadrantBottomLeft) ? _splitPane.vertPane1 : _splitPane.vertPane2;

        if (quadrant.type == Quadrant::Camera)
        {
            _camWnd = GlobalCamera().createCamWnd(parent);
            quadrant.widget = _camWnd->getMainWidget();
        }
        else
        {
            quadrant.xyWnd = GlobalXYWnd().createEmbeddedOrthoView(XY, parent);
            quadrant.widget = quadrant.xyWnd->getGLWidget();
        }

        _quadrants[pos] = quadrant;
    }

    // Call Split on the wxSplitterWindows to complete creation
    _splitPane.vertPane1->SplitHorizontally(_quadrants[QuadrantTopLeft].widget, _quadrants[QuadrantBottomLeft].widget);
    _splitPane.vertPane2->SplitHorizontally(_quadrants[QuadrantTopRight].widget, _quadrants[QuadrantBottomRight].widget);
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

void SplitPaneLayout::restoreStateFromRegistry()
{
	restoreStateFromPath(RKEY_SPLITPANE_ROOT);
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

// The creation function, needed by the mainframe layout manager
SplitPaneLayoutPtr SplitPaneLayout::CreateInstance() {
	return SplitPaneLayoutPtr(new SplitPaneLayout);
}

} // namespace ui
