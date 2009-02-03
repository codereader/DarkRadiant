#include "MainFrame.h"

#include <gtk/gtk.h>

#include "RadiantModule.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"
#include "iregistry.h"
#include "igrid.h"

#include "ui/splash/Splash.h"
#include "ui/menu/FiltersMenu.h"
#include "log/Console.h"
#include "xyview/GlobalXYWnd.h"
#include "windowobservers.h"
#include "ui/einspector/EntityInspector.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/layers/LayerControlDialog.h"
#include "ui/overlay/Overlay.h"
#include "camera/GlobalCamera.h"
#include "camera/CameraSettings.h"
#include "selection/shaderclipboard/ShaderClipboard.h"

#include "map/AutoSaver.h"
#include "brush/BrushModule.h"
#include "gtkutil/window.h"
#include "gtkutil/Paned.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/window/PersistentTransientWindow.h"

#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/mainframe/FloatingLayout.h"

#include "modulesystem/StaticModule.h"

	namespace {
		const std::string RKEY_WINDOW_LAYOUT = "user/ui/mainFrame/windowLayout";
		const std::string RKEY_WINDOW_STATE = "user/ui/mainFrame/window";
		const std::string RKEY_MULTIMON_START_PRIMARY = "user/ui/multiMonitor/startOnPrimaryMonitor";

		const std::string RKEY_ACTIVE_LAYOUT = "user/ui/mainFrame/activeLayout";
	}

namespace ui {

MainFrame::MainFrame() : 
	_window(NULL),
	_screenUpdatesEnabled(true)
{}

// RegisterableModule implementation
const std::string& MainFrame::getName() const {
	static std::string _name(MODULE_MAINFRAME);
	return _name;
}

const StringSet& MainFrame::getDependencies() const {
	static StringSet _dependencies;
	
	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_MAINFRAME_LAYOUT_MANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_UIMANAGER);
	}
	
	return _dependencies;
}

void MainFrame::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "MainFrame::initialiseModule called.\n";
}

void MainFrame::shutdownModule() {
	globalOutputStream() << "MainFrame::shutdownModule called.\n";
}

void MainFrame::construct() {
	// Create the base window and the default widgets
	create();

	std::string activeLayout = GlobalRegistry().get(RKEY_ACTIVE_LAYOUT);

	if (activeLayout.empty()) {
		activeLayout = FLOATING_LAYOUT_NAME; // fall back to hardcoded layout
	}

	// Apply the layout
	applyLayout(activeLayout);

	if (_currentLayout == NULL) {
		// Layout is still empty, this is not good
		globalErrorStream() << "Could not restore layout " << activeLayout << std::endl;

		if (activeLayout != FLOATING_LAYOUT_NAME) {
			// Try to fallback to floating layout
			applyLayout(FLOATING_LAYOUT_NAME);
		}
	}
  
  	// Broadcast the startup event
    radiant::getGlobalRadiant()->broadcastStartupEvent();
}

void MainFrame::destroy() {

	saveWindowInfo();

	// Free the layout
	if (_currentLayout != NULL) {
		// Save the layout to the registry
		GlobalRegistry().set(RKEY_ACTIVE_LAYOUT, _currentLayout->getName());

		_currentLayout->deactivate();
		_currentLayout = IMainFrameLayoutPtr();
	}	
	
	gtk_widget_hide(GTK_WIDGET(_window));
	
	shutdown();

	gtk_widget_destroy(GTK_WIDGET(_window));
}

GtkWindow* MainFrame::getTopLevelWindow() {
	return _window;
}

GtkWindow* MainFrame::createTopLevelWindow() {
	// Destroy any previous toplevel window
	if (_window != NULL) {
		GlobalEventManager().disconnect(GTK_OBJECT(_window));

		// Clear the module as well
		radiant::getGlobalRadiant()->setMainWindow(NULL);

		gtk_widget_hide(GTK_WIDGET(_window));
		gtk_widget_destroy(GTK_WIDGET(_window));
	}

	// Create a new window
	_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	
	// Let the radiant module know about the new toplevel
	radiant::getGlobalRadiant()->setMainWindow(_window);

	// Tell the XYManager which window the xyviews should be transient for
	GlobalXYWnd().setGlobalParentWindow(_window);

	GlobalWindowObservers_connectTopLevel(_window);

	// Set the splash window transient to this toplevel
	gtk_window_set_transient_for(ui::Splash::Instance().getWindow(), _window);

#if !defined(WIN32)
	{
		// Set the default icon for non-Win32-systems 
		// (Win32 builds use the one embedded in the exe)
		std::string icon = GlobalRegistry().get(RKEY_BITMAPS_PATH) + 
  						   "darkradiant_icon_64x64.png";
		gtk_window_set_default_icon_from_file(icon.c_str(),	NULL);
	}
#endif

	// Signal setup
	gtk_widget_add_events(GTK_WIDGET(_window), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(_window), "delete-event", G_CALLBACK(onDelete), this);

	// Notify the event manager
	GlobalEventManager().connect(GTK_OBJECT(_window));
    GlobalEventManager().connectAccelGroup(_window);

	return _window;
}

void MainFrame::restoreWindowPosition() {
	// We start out maximised by default
	int windowState = GDK_WINDOW_STATE_MAXIMIZED;

	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
		windowState = strToInt(windowStateList[0].getAttributeValue("state"));
	}
	
#ifdef WIN32
	// Do the settings say that we should start on the primary screen?
	if (GlobalRegistry().get(RKEY_MULTIMON_START_PRIMARY) == "1") {
		// Yes, connect the position tracker, this overrides the existing setting.
  		_windowPosition.connect(_window);
  		// Load the correct coordinates into the position tracker
		_windowPosition.fitToScreen(gtkutil::MultiMonitor::getMonitor(0));
		// Apply the position
		_windowPosition.applyPosition();
	}
	else
#endif
	if (windowState & GDK_WINDOW_STATE_MAXIMIZED) {
		gtk_window_maximize(_window);
	}
	else {
		_windowPosition.connect(_window);
		_windowPosition.applyPosition();
	}
}

GtkWidget* MainFrame::createMenuBar() {
	// Create the Filter menu entries before adding the menu bar
    ui::FiltersMenu::addItems();
    
    // Get the reference to the MenuManager class
    IMenuManager& menuManager = GlobalUIManager().getMenuManager();
    
    // Retrieve the "main" menubar from the UIManager
    GtkWidget* mainMenu = menuManager.get("main");
    
    if (m_nCurrentStyle != eFloating) {
    	// Hide the camera toggle option for non-floating views
    	menuManager.setVisibility("main/view/cameraview", false);
    }
    
	if (m_nCurrentStyle != eFloating && m_nCurrentStyle != eSplit) {
		// Hide the console/texture browser toggles for non-floating/non-split views
		menuManager.setVisibility("main/view/consoleView", false);
		menuManager.setVisibility("main/view/textureBrowser", false);	
	}

	return mainMenu;
}

void MainFrame::create() {
	// Create the topmost window first
	createTopLevelWindow();

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(_window), vbox);
    gtk_widget_show(vbox);
    
    int viewStyle = GlobalRegistry().getInt(RKEY_WINDOW_LAYOUT);
    
    switch (viewStyle) {
    	case 0: m_nCurrentStyle = eRegular; break;
    	case 1: m_nCurrentStyle = eFloating; break;
    	case 2: m_nCurrentStyle = eSplit; break;
    	case 3: m_nCurrentStyle = eRegularLeft; break;
    	default: m_nCurrentStyle = eFloating; break;
    };

    // Retrieve the "main" menubar from the UIManager
    gtk_box_pack_start(GTK_BOX(vbox), createMenuBar(), FALSE, FALSE, 0);
    
    // Instantiate the ToolbarManager and retrieve the view toolbar widget 
	IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();
	
	GtkToolbar* viewToolbar = tbCreator.getToolbar("view");
	if (viewToolbar != NULL) {
		// Pack it into the main window
		gtk_widget_show(GTK_WIDGET(viewToolbar));
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(viewToolbar), FALSE, FALSE, 0);
	}
	
    GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    
    // Get the edit toolbar widget 
	GtkToolbar* editToolbar = tbCreator.getToolbar("edit");
	if (editToolbar != NULL) {
		// Pack it into the main window
		gtk_widget_show(GTK_WIDGET(editToolbar));
		gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(editToolbar), FALSE, FALSE, 0);
	}
    
    // Create and pack main statusbar 
    GtkWidget* statusBar = GlobalUIManager().getStatusBarManager().getStatusBar();
    gtk_box_pack_end(GTK_BOX(vbox), statusBar, FALSE, TRUE, 2);
	gtk_widget_show_all(statusBar);

	/* Construct the Group Dialog. This is the tabbed window that contains
     * a number of pages - usually Entities, Textures and possibly Console.
     */
    // Add entity inspector widget
    GlobalGroupDialog().addPage(
    	"entity",	// name
    	"Entity", // tab title
    	"cmenu_add_entity.png", // tab icon 
    	ui::EntityInspector::getInstance().getWidget(), // page widget
    	"Entity"
    );

	// Add the Media Browser page
	GlobalGroupDialog().addPage(
    	"mediabrowser",	// name
    	"Media", // tab title
    	"folder16.png", // tab icon 
    	ui::MediaBrowser::getInstance().getWidget(), // page widget
    	"Media"
    );
	
    // Add the console widget if using floating window mode, otherwise the
    // console is placed in the bottom-most split pane.
    if (FloatingGroupDialog()) {
    	GlobalGroupDialog().addPage(
	    	"console",	// name
	    	"Console", // tab title
	    	"iconConsole16.png", // tab icon 
			ui::Console::Instance().getWidget(), // page widget
	    	"Console"
	    );
    }

	// Load the previous window settings from the registry
	restoreWindowPosition();

	gtk_widget_show(GTK_WIDGET(_window));

	// Create the camera instance
	GlobalCamera().setParent(_window);

#if 0
	CamWndPtr camWnd = GlobalCamera().getCamWnd();
	
	if (CurrentStyle() == eRegular || CurrentStyle() == eRegularLeft) {
    	// Allocate a new OrthoView and set its ViewType to XY
		XYWndPtr xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
        xyWnd->setViewType(XY);
        // Create a framed window out of the view's internal widget
        GtkWidget* xyView = gtkutil::FramedWidget(xyWnd->getWidget());

        // Pack in the camera window
		GtkWidget* camWindow = gtkutil::FramedWidget(camWnd->getWidget());
		// greebo: The mainframe window acts as parent for the camwindow
	    camWnd->setContainer(_window);

        // Create the texture window
		GtkWidget* texWindow = gtkutil::FramedWidget(
			GlobalTextureBrowser().constructWindow(_window)
		);

        // Create the Console
		GtkWidget* console = ui::Console::Instance().getWidget();
        
        // Now pack those widgets into the paned widgets

        // First, pack the texwindow and the camera
        _regular.texCamPane = gtkutil::Paned(camWindow, texWindow, false);
        
        // Depending on the viewstyle, pack the xy left or right
        if (CurrentStyle() == eRegularLeft) {
        	_regular.horizPane = gtkutil::Paned(_regular.texCamPane, xyView, true);
        }
        else {
        	// This is "regular", put the xyview to the left
        	_regular.horizPane = gtkutil::Paned(xyView, _regular.texCamPane, true);
        }
        
        // Finally, pack the horizontal pane plus the console window into a vpane
        _regular.vertPane = gtkutil::Paned(_regular.horizPane, console, false);
        
        // Add this to the mainframe hbox
		gtk_box_pack_start(GTK_BOX(hbox), _regular.vertPane, TRUE, TRUE, 0);
        gtk_widget_show_all(_regular.vertPane);

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
	
		if (list.size() > 0) {
			_regular.posVPane.loadFromNode(list[0]);
			_regular.posVPane.applyPosition();
		}
	
		list = GlobalRegistry().findXPath("user/ui/mainFrame/regular/pane[@name='horizontal']");
	
		if (list.size() > 0) {
			_regular.posHPane.loadFromNode(list[0]);
			_regular.posHPane.applyPosition();
		}
	
		list = GlobalRegistry().findXPath("user/ui/mainFrame/regular/pane[@name='texcam']");

		if (list.size() > 0) {
			_regular.posTexCamPane.loadFromNode(list[0]);
			_regular.posTexCamPane.applyPosition();
		}
	} // end if (regular)
  else if (CurrentStyle() == eFloating)
  {
	  
    {
     	// Get the floating window with the CamWnd packed into it
		gtkutil::PersistentTransientWindowPtr floatingWindow =
			GlobalCamera().getFloatingWindow();
		GlobalEventManager().connectAccelGroup(
			GTK_WINDOW(floatingWindow->getWindow()));
      
		floatingWindow->show();
    }

   	{
		GtkWidget* page = gtkutil::FramedWidget(
			GlobalTextureBrowser().constructWindow(GTK_WINDOW(GlobalGroupDialog().getDialogWindow()))
		);
		// Add the Media Browser page
		GlobalGroupDialog().addPage(
	    	"textures",	// name
	    	"Textures", // tab title
	    	"icon_texture.png", // tab icon 
	    	GTK_WIDGET(page), // page widget
	    	"Texture Browser"
	    );
    }

    GlobalGroupDialog().showDialogWindow();
	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload 
	// the position info from the Registry once again.
	ui::EntityInspector::getInstance().restoreSettings();
  }
  else // 4 way (aka Splitplane view)
  {
    GtkWidget* camera = camWnd->getWidget();
    // greebo: The mainframe window acts as parent for the camwindow
    camWnd->setContainer(_window);

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

	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(_splitPane.horizPane), TRUE, TRUE, 0);

	gtk_paned_set_position(GTK_PANED(_splitPane.horizPane), 200);
	gtk_paned_set_position(GTK_PANED(_splitPane.vertPane1), 200);
	gtk_paned_set_position(GTK_PANED(_splitPane.vertPane2), 400);

	_splitPane.posHPane.connect(_splitPane.horizPane);
	_splitPane.posVPane1.connect(_splitPane.vertPane1);
	_splitPane.posVPane2.connect(_splitPane.vertPane2);
	
	// TODO: Move this whole stuff into a class (maybe some deriving from a Layout class)
	xml::NodeList list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='horizontal']");
	
	if (list.size() > 0) {
		_splitPane.posHPane.loadFromNode(list[0]);
		_splitPane.posHPane.applyPosition();
	}
	
	list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='vertical1']");
	
	if (list.size() > 0) {
		_splitPane.posVPane1.loadFromNode(list[0]);
		_splitPane.posVPane1.applyPosition();
	}
	
	list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='vertical2']");
	
	if (list.size() > 0) {
		_splitPane.posVPane2.loadFromNode(list[0]);
		_splitPane.posVPane2.applyPosition();
	}
	
    {      
		GtkWidget* textureBrowser = gtkutil::FramedWidget(
			GlobalTextureBrowser().constructWindow(_window)
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
  }
#endif

	// Start the autosave timer so that it can periodically check the map for changes 
	map::AutoSaver().startTimer();
  
	// Restore any floating XYViews that were active before, this applies to all view layouts
	GlobalXYWnd().restoreState();
	
	// Initialise the shaderclipboard
	GlobalShaderClipboard().clear();

	ui::LayerControlDialog::init();
}

void MainFrame::saveWindowInfo() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	node.setAttributeValue(
		"state", 
		intToStr(gdk_window_get_state(GTK_WIDGET(_window)->window))
	); 
  
	// Save the splitpane widths/heights 
	if (CurrentStyle() == eSplit) {
		// TODO: Move this whole stuff into a class 
		// (maybe some deriving from a Layout class)
		
		std::string path("user/ui/mainFrame/splitPane");
		
		// Remove all previously stored pane information 
		GlobalRegistry().deleteXPath(path + "//pane");
		
		xml::Node node = GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
		_splitPane.posHPane.saveToNode(node);
		
		node = GlobalRegistry().createKeyWithName(path, "pane", "vertical1");
		_splitPane.posVPane1.saveToNode(node);
		
		node = GlobalRegistry().createKeyWithName(path, "pane", "vertical2");
		_splitPane.posVPane2.saveToNode(node);
	}
	else if (CurrentStyle() == eRegular || CurrentStyle() == eRegularLeft) {
		std::string path("user/ui/mainFrame/regular");
		
		// Remove all previously stored pane information 
		GlobalRegistry().deleteXPath(path + "//pane");
		
		xml::Node node = GlobalRegistry().createKeyWithName(path, "pane", "vertical");
		_regular.posVPane.saveToNode(node);
		
		node = GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
		_regular.posHPane.saveToNode(node);
		
		node = GlobalRegistry().createKeyWithName(path, "pane", "texcam");
		_regular.posTexCamPane.saveToNode(node);
	} 
}

void MainFrame::shutdown()
{
	// Shutdown the console
	ui::Console::Instance().shutdown();

	// Shutdown the texturebrowser (before the GroupDialog gets shut down).
	GlobalTextureBrowser().destroyWindow();
	
	// Broadcast shutdown event to RadiantListeners
	radiant::getGlobalRadiant()->broadcastShutdownEvent();

	// Destroy the Overlay instance
	ui::Overlay::destroyInstance();
	
	// Stop the AutoSaver class from being called
	map::AutoSaver().stopTimer();

  	// Save the camera size to the registry
	GlobalCamera().saveCamWndState();
	
	// Save the current XYViews to the registry
	GlobalXYWnd().saveState();
	GlobalXYWnd().destroyViews();
	
	GlobalCamera().destroy();
	GlobalXYWnd().destroy();
}

bool MainFrame::screenUpdatesEnabled() {
	return _screenUpdatesEnabled;
}

void MainFrame::enableScreenUpdates() {
	_screenUpdatesEnabled = true;
}

void MainFrame::disableScreenUpdates() {
	_screenUpdatesEnabled = false;
}

void MainFrame::updateAllWindows() {
	GlobalCamera().update();
	GlobalXYWnd().updateAllViews();
}

void MainFrame::applyLayout(const std::string& name) {
	// Try to find that new layout
	IMainFrameLayoutPtr layout = GlobalMainFrameLayoutManager().getLayout(name);

	if (layout == NULL) {
		globalErrorStream() << "MainFrame: Could not find layout with name " << name << std::endl;
		return;
	}

	// Deactivate the current layout first
	if (_currentLayout != NULL) {
		_currentLayout->deactivate();		
	}

	globalOutputStream() << "MainFrame: Activating layout " << name << std::endl;

	// Store and activate the new layout
	_currentLayout = layout;
	_currentLayout->activate();
}

// GTK callbacks
gboolean MainFrame::onDelete(GtkWidget* widget, GdkEvent* ev, MainFrame* self) {
	if (GlobalMap().askForSave("Exit Radiant")) {
		gtk_main_quit();
	}

	return TRUE;
}

// Define the static MainFrame module
module::StaticModule<MainFrame> mainFrameModule;

} // namespace ui
