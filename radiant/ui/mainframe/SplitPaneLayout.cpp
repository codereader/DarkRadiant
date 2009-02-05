#include "SplitPaneLayout.h"

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

namespace ui {

std::string SplitPaneLayout::getName() {
	return SPLITPANE_LAYOUT_NAME;
}

void SplitPaneLayout::activate() {

	GtkWindow* parent = GlobalMainFrame().getTopLevelWindow();

	// Create a new camera window and parent it
	_camWnd = GlobalCamera().createCamWnd();
	 // greebo: The mainframe window acts as parent for the camwindow
	_camWnd->setContainer(parent);

	GtkWidget* camera = _camWnd->getWidget();

	// Allocate the three ortho views
    XYWndPtr xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
    xyWnd->setViewType(XY);
    GtkWidget* xy = xyWnd->getWidget();
    
    XYWndPtr yzWnd = GlobalXYWnd().createEmbeddedOrthoView();
    yzWnd->setViewType(YZ);
    GtkWidget* yz = yzWnd->getWidget();

    XYWndPtr xzWnd = GlobalXYWnd().createEmbeddedOrthoView();
    xzWnd->setViewType(XZ);
    GtkWidget* xz = xzWnd->getWidget();

	// Arrange the widgets into the paned views
	_splitPane.vertPane1 = gtkutil::Paned(gtkutil::FramedWidget(camera), 
										  gtkutil::FramedWidget(yz), 
										  false);
	_splitPane.vertPane2 = gtkutil::Paned(gtkutil::FramedWidget(xy), 
										  gtkutil::FramedWidget(xz), 
										  false);
	_splitPane.horizPane = gtkutil::Paned(_splitPane.vertPane1, _splitPane.vertPane2, true);

	// Retrieve the main container of the main window
	GtkWidget* mainContainer = GlobalMainFrame().getMainContainer();
	gtk_container_add(GTK_CONTAINER(mainContainer), GTK_WIDGET(_splitPane.horizPane));

	gtk_paned_set_position(GTK_PANED(_splitPane.horizPane), 200);
	gtk_paned_set_position(GTK_PANED(_splitPane.vertPane1), 200);
	gtk_paned_set_position(GTK_PANED(_splitPane.vertPane2), 400);

	_splitPane.posHPane.connect(_splitPane.horizPane);
	_splitPane.posVPane1.connect(_splitPane.vertPane1);
	_splitPane.posVPane2.connect(_splitPane.vertPane2);
	
	xml::NodeList list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='horizontal']");
	
	if (!list.empty()) {
		_splitPane.posHPane.loadFromNode(list[0]);
		_splitPane.posHPane.applyPosition();
	}
	
	list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='vertical1']");
	
	if (!list.empty()) {
		_splitPane.posVPane1.loadFromNode(list[0]);
		_splitPane.posVPane1.applyPosition();
	}
	
	list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='vertical2']");
	
	if (!list.empty()) {
		_splitPane.posVPane2.loadFromNode(list[0]);
		_splitPane.posVPane2.applyPosition();
	}
	
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
	    	"Texture Browser"
	    );
    }

	GlobalGroupDialog().showDialogWindow();

	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload 
	// the position info from the Registry once again.
	ui::EntityInspector::getInstance().restoreSettings();

	GlobalGroupDialog().hideDialogWindow();

	gtk_widget_show_all(mainContainer);
}

void SplitPaneLayout::deactivate() {
	// Save the splitplane info
	std::string path("user/ui/mainFrame/splitPane");
		
	// Remove all previously stored pane information 
	GlobalRegistry().deleteXPath(path + "//pane");
	
	xml::Node node = GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
	_splitPane.posHPane.saveToNode(node);
	
	node = GlobalRegistry().createKeyWithName(path, "pane", "vertical1");
	_splitPane.posVPane1.saveToNode(node);
	
	node = GlobalRegistry().createKeyWithName(path, "pane", "vertical2");
	_splitPane.posVPane2.saveToNode(node);

	// Delete all active views
	GlobalXYWnd().destroyViews();

	// Delete the CamWnd
	_camWnd = CamWndPtr();

	// Hide the group dialog
	GlobalGroupDialog().hideDialogWindow();

	// Remove the texture browser from the groupdialog
	GlobalGroupDialog().removePage("textures");
	GlobalTextureBrowser().destroyWindow();

	// Destroy the widget, so it gets removed from the main container
	gtk_widget_destroy(GTK_WIDGET(_splitPane.horizPane));
}

// The creation function, needed by the mainframe layout manager
SplitPaneLayoutPtr SplitPaneLayout::CreateInstance() {
	return SplitPaneLayoutPtr(new SplitPaneLayout);
}

} // namespace ui
