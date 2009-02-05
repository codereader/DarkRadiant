#include "RegularLayout.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "imainframe.h"

#include "gtkutil/FramedWidget.h"
#include "gtkutil/Paned.h"

#include "camera/GlobalCamera.h"
#include "ui/einspector/EntityInspector.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "xyview/GlobalXYWnd.h"
#include "log/Console.h"

namespace ui {

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

	// Create the Console
	GtkWidget* console = Console::Instance().getWidget();

	// Now pack those widgets into the paned widgets

	// First, pack the texwindow and the camera
    _regular.texCamPane = gtkutil::Paned(camWindow, texWindow, false);
    
    // Depending on the viewstyle, pack the xy left or right
    if (_regularLeft) {
    	_regular.horizPane = gtkutil::Paned(_regular.texCamPane, xyView, true);
    }
    else {
    	// This is "regular", put the xyview to the left
    	_regular.horizPane = gtkutil::Paned(xyView, _regular.texCamPane, true);
    }
    
    // Finally, pack the horizontal pane plus the console window into a vpane
    _regular.vertPane = gtkutil::Paned(_regular.horizPane, console, false);

	// Retrieve the main container of the main window
	GtkWidget* mainContainer = GlobalMainFrame().getMainContainer();
	gtk_container_add(GTK_CONTAINER(mainContainer), GTK_WIDGET(_regular.vertPane));

	// Set some default values for the width and height
    gtk_paned_set_position(GTK_PANED(_regular.vertPane), 650);
	gtk_paned_set_position(GTK_PANED(_regular.horizPane), 500);
	gtk_paned_set_position(GTK_PANED(_regular.texCamPane), 350);

	// Connect the pane position trackers
	_regular.posVPane.connect(_regular.vertPane);
	_regular.posHPane.connect(_regular.horizPane);
	_regular.posTexCamPane.connect(_regular.texCamPane);
	
	// Now load the paned positions from the registry
	xml::NodeList list = GlobalRegistry().findXPath("user/ui/mainFrame/regular/pane[@name='vertical']");

	if (!list.empty()) {
		_regular.posVPane.loadFromNode(list[0]);
		_regular.posVPane.applyPosition();
	}

	list = GlobalRegistry().findXPath("user/ui/mainFrame/regular/pane[@name='horizontal']");

	if (!list.empty()) {
		_regular.posHPane.loadFromNode(list[0]);
		_regular.posHPane.applyPosition();
	}

	list = GlobalRegistry().findXPath("user/ui/mainFrame/regular/pane[@name='texcam']");

	if (!list.empty()) {
		_regular.posTexCamPane.loadFromNode(list[0]);
		_regular.posTexCamPane.applyPosition();
	}
	
    GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload 
	// the position info from the Registry once again.
	ui::EntityInspector::getInstance().restoreSettings();

	GlobalGroupDialog().hideDialogWindow();

	gtk_widget_show_all(mainContainer);
}

void RegularLayout::deactivate() {
	std::string path("user/ui/mainFrame/regular");
		
	// Remove all previously stored pane information 
	GlobalRegistry().deleteXPath(path + "//pane");
	
	xml::Node node = GlobalRegistry().createKeyWithName(path, "pane", "vertical");
	_regular.posVPane.saveToNode(node);
	
	node = GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
	_regular.posHPane.saveToNode(node);
	
	node = GlobalRegistry().createKeyWithName(path, "pane", "texcam");
	_regular.posTexCamPane.saveToNode(node);

	// Delete all active views
	GlobalXYWnd().destroyViews();

	// Delete the CamWnd
	_camWnd = CamWndPtr();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	GlobalTextureBrowser().destroyWindow();

	// Destroy the widget, so it gets removed from the main container
	gtk_widget_destroy(GTK_WIDGET(_regular.vertPane));
}

// The creation function, needed by the mainframe layout manager
RegularLayoutPtr RegularLayout::CreateRegularLeftInstance() {
	return RegularLayoutPtr(new RegularLayout(true));
}

RegularLayoutPtr RegularLayout::CreateRegularInstance() {
	return RegularLayoutPtr(new RegularLayout(false));
}

} // namespace ui
