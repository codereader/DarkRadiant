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
#include <wx/splitter.h>
#include <boost/bind.hpp>

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

static void draw()
{
	glViewport(0, 0, 60, 60);

    // enable depth buffer writes
    glDepthMask(GL_TRUE);

    Vector3 clearColour(0, 0, 0);
    glClearColor(clearColour[0], clearColour[1], clearColour[2], 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void EmbeddedLayout::activate()
{
	wxFrame* topLevelParent = GlobalMainFrame().getWxTopLevelWindow();

	// Splitters
	_horizPane = new wxSplitterWindow(topLevelParent, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "EmbeddedHorizPane");

	_horizPane->SetSashGravity(0.5);
	_horizPane->SetSashPosition(400);

	GlobalMainFrame().getWxMainContainer()->Add(_horizPane, 1, wxEXPAND);

	// Allocate a new OrthoView and set its ViewType to XY
	XYWndPtr xywnd = GlobalXYWnd().createEmbeddedOrthoView(XY, _horizPane);

	// CamGroup Pane
	_groupCamPane = new wxSplitterWindow(_horizPane, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D | wxWANTS_CHARS, "EmbeddedVertPane");

	_groupCamPane->SetSashGravity(0.5);
	_groupCamPane->SetSashPosition(300);

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd(_groupCamPane);

	wxPanel* notebookPanel = new wxPanel(_groupCamPane, wxID_ANY);
	notebookPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	GlobalGroupDialog().reparentNotebook(notebookPanel);

	// Hide the floating window
	GlobalGroupDialog().hideDialogWindow();

	// Add a new texture browser to the group dialog pages
	wxWindow* textureBrowser = GlobalTextureBrowser().constructWindow(notebookPanel);

	// Texture Page
	{
		IGroupDialog::PagePtr page(new IGroupDialog::Page);

		page->name = "textures";
		page->windowLabel = _("Texture Browser");
		page->widget = textureBrowser;
		page->tabIcon = "icon_texture.png";
		page->tabLabel = _("Textures");

		GlobalGroupDialog().addWxPage(page);
	}

	_groupCamPane->SplitHorizontally(_camWnd->getMainWidget(), notebookPanel);
	
	// Add the camGroup pane to the left and the GL widget to the right
	_horizPane->SplitVertically(_groupCamPane, xywnd->getGLWidget());
	
	// Connect the pane position trackers
	_posHPane.connect(_horizPane);
	_posGroupCamPane.connect(_groupCamPane);

	// Attempt to restore this layout's state
	restoreStateFromPath(RKEY_EMBEDDED_ROOT);

#if 0
	// GTK stuff

	// Get the toplevel window
	const Glib::RefPtr<Gtk::Window>& parent = GlobalMainFrame().getTopLevelWindow();

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd(topLevelParent);
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
#endif

	// wxTODO

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

	// Disconnect before destroying stuff
	_posHPane.disconnect(_horizPane);
	_posGroupCamPane.disconnect(_groupCamPane);

	wxFrame* topLevelParent = GlobalMainFrame().getWxTopLevelWindow();
	topLevelParent->RemoveChild(_horizPane);

	// Those two have been deleted by the above, so NULL the references
	_horizPane = NULL;
	_groupCamPane = NULL;
}

void EmbeddedLayout::maximiseCameraSize()
{
	// Save the current state to the registry
	saveStateToPath(RKEY_EMBEDDED_TEMP_ROOT);

	// Maximise the camera, wxWidgets will clip the coordinates
	_horizPane->SetSashPosition(2000000);
	_groupCamPane->SetSashPosition(2000000);
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
	// Trigger a proper resize event before setting the sash position
	GlobalMainFrame().getWxTopLevelWindow()->SendSizeEvent();

	// Now load the paned positions from the registry
	if (GlobalRegistry().keyExists(path + "/pane[@name='texcam']"))
	{
		_posGroupCamPane.loadFromPath(path + "/pane[@name='texcam']");
		_posGroupCamPane.applyPosition();
	}

	if (GlobalRegistry().keyExists(path + "/pane[@name='horizontal']"))
	{
		_posHPane.loadFromPath(path + "/pane[@name='horizontal']");
		_posHPane.applyPosition();
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
