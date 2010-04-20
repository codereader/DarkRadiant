#include "MainFrame.h"

#include <gtk/gtk.h>

#include "i18n.h"
#include "RadiantModule.h"
#include "iuimanager.h"
#include "idialogmanager.h"
#include "igroupdialog.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"
#include "igrid.h"
#include "ientityinspector.h"

#include "ui/splash/Splash.h"
#include "ui/menu/FiltersMenu.h"
#include "log/Console.h"
#include "xyview/GlobalXYWnd.h"
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
#include "ui/mainframe/EmbeddedLayout.h"

#include "modulesystem/StaticModule.h"
#include <boost/bind.hpp>
#include <boost/format.hpp>

#ifdef WIN32
#include <windows.h>
#endif

	namespace {
		const std::string RKEY_WINDOW_LAYOUT = "user/ui/mainFrame/windowLayout";
		const std::string RKEY_WINDOW_STATE = "user/ui/mainFrame/window";
		const std::string RKEY_MULTIMON_START_MONITOR = "user/ui/multiMonitor/startMonitorNum";
		const std::string RKEY_DISABLE_WIN_DESKTOP_COMP = "user/ui/compatibility/disableWindowsDesktopComposition";

		const std::string RKEY_ACTIVE_LAYOUT = "user/ui/mainFrame/activeLayout";
	}

namespace ui {

MainFrame::MainFrame() : 
	_window(NULL),
	_mainContainer(NULL),
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
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
	}
	
	return _dependencies;
}

void MainFrame::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << "MainFrame::initialiseModule called." << std::endl;

	// Add another page for Multi-Monitor stuff
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Multi Monitor"));

	// Initialise the registry, if no key is set
	if (GlobalRegistry().get(RKEY_MULTIMON_START_MONITOR).empty())
	{
		GlobalRegistry().set(RKEY_MULTIMON_START_MONITOR, "0");
	}

	ComboBoxValueList list;

	for (int i = 0; i < gtkutil::MultiMonitor::getNumMonitors(); ++i)
	{
		GdkRectangle rect = gtkutil::MultiMonitor::getMonitor(i);

		list.push_back(
			(boost::format("Monitor %d (%dx%d)") % i % rect.width % rect.height).str()
		);
	}

	page->appendCombo(_("Start DarkRadiant on monitor"), RKEY_MULTIMON_START_MONITOR, list);

	// Add the toggle max/min command for floating windows
	GlobalCommandSystem().addCommand("ToggleFullScreenCamera", 
		boost::bind(&MainFrame::toggleFullscreenCameraView, this, _1)
	);
	GlobalEventManager().addCommand("ToggleFullScreenCamera", "ToggleFullScreenCamera");

#ifdef WIN32
	HMODULE lib = LoadLibrary("dwmapi.dll");

	if (lib != NULL)
	{
		void (WINAPI *dwmEnableComposition) (bool) = 
			(void (WINAPI *) (bool)) GetProcAddress(lib, "DwmEnableComposition");

		if (dwmEnableComposition)
		{
			// Add a page for Desktop Composition stuff
			PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Compatibility"));

			page->appendCheckBox("", _("Disable Windows Desktop Composition"), 
				RKEY_DISABLE_WIN_DESKTOP_COMP);

			GlobalRegistry().addKeyObserver(this, RKEY_DISABLE_WIN_DESKTOP_COMP);
		}
		
		FreeLibrary(lib);
	}

	// Load the value and act
	setDesktopCompositionEnabled(GlobalRegistry().get(RKEY_DISABLE_WIN_DESKTOP_COMP) != "1");
#endif
}

void MainFrame::shutdownModule()
{
	globalOutputStream() << "MainFrame::shutdownModule called." << std::endl;
}

void MainFrame::keyChanged(const std::string& changedKey, const std::string& newValue)
{
#ifdef WIN32
	if (changedKey == RKEY_DISABLE_WIN_DESKTOP_COMP)
	{
		setDesktopCompositionEnabled(newValue != "1");
	}
#endif
}

#ifdef WIN32
void MainFrame::setDesktopCompositionEnabled(bool enabled)
{
	HMODULE lib = LoadLibrary("dwmapi.dll");

	if (lib != NULL)
	{
		void (WINAPI *dwmEnableComposition) (bool) = 
			(void (WINAPI *) (bool)) GetProcAddress(lib, "DwmEnableComposition");

		if (dwmEnableComposition)
		{
			dwmEnableComposition(enabled ? TRUE : FALSE);
		}
		
		FreeLibrary(lib);
	}
}
#endif

void MainFrame::toggleFullscreenCameraView(const cmd::ArgumentList& args)
{
	if (_currentLayout == NULL) return;

	// Issue the call
	_currentLayout->toggleFullscreenCameraView();
}

void MainFrame::construct()
{
	// Create the base window and the default widgets
	create();

	std::string activeLayout = GlobalRegistry().get(RKEY_ACTIVE_LAYOUT);

	if (activeLayout.empty()) {
		activeLayout = EMBEDDED_LAYOUT_NAME; // fall back to hardcoded layout
	}

	// Apply the layout
	applyLayout(activeLayout);

	if (_currentLayout == NULL)
	{
		// Layout is still empty, this is not good
		globalErrorStream() << "Could not restore layout " << activeLayout << std::endl;

		if (activeLayout != EMBEDDED_LAYOUT_NAME) {
			// Try to fallback to floating layout
			applyLayout(EMBEDDED_LAYOUT_NAME);
		}
	}

	// register the commands
	GlobalMainFrameLayoutManager().registerCommands();
  
  	// Broadcast the startup event
    radiant::getGlobalRadiant()->broadcastStartupEvent();
}

void MainFrame::removeLayout()
{
	// Sanity check
	if (_currentLayout == NULL) return;

	_currentLayout->deactivate();
	_currentLayout = IMainFrameLayoutPtr();	
}

void MainFrame::destroy() {

	saveWindowPosition();

	// Free the layout
	if (_currentLayout != NULL) {
		// Save the layout to the registry
		GlobalRegistry().set(RKEY_ACTIVE_LAYOUT, _currentLayout->getName());

		removeLayout();
	}
	
	gtk_widget_hide(GTK_WIDGET(_window));
	
	shutdown();

	gtk_widget_destroy(GTK_WIDGET(_window));
}

GtkWindow* MainFrame::getTopLevelWindow() {
	return _window;
}

GtkWidget* MainFrame::getMainContainer() {
	return _mainContainer;
}

GtkWindow* MainFrame::createTopLevelWindow()
{
	// Destroy any previous toplevel window
	if (_window != NULL)
	{
		GlobalEventManager().disconnect(GTK_OBJECT(_window));

		gtk_widget_hide(GTK_WIDGET(_window));
		gtk_widget_destroy(GTK_WIDGET(_window));

		_window = NULL;
	}

	// Create a new window
	_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	
	// Tell the XYManager which window the xyviews should be transient for
	GlobalXYWnd().setGlobalParentWindow(_window);

	// Set the splash window transient to this toplevel
	gtk_window_set_transient_for(Splash::Instance().getWindow(), _window);

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

void MainFrame::restoreWindowPosition()
{
	// We start out maximised by default
	int windowState = GDK_WINDOW_STATE_MAXIMIZED;

	// Connect the window position tracker
	if (!GlobalRegistry().findXPath(RKEY_WINDOW_STATE).empty())
	{
		_windowPosition.loadFromPath(RKEY_WINDOW_STATE);
		windowState = strToInt(GlobalRegistry().getAttribute(RKEY_WINDOW_STATE, "state"));
	}

	int startMonitor = GlobalRegistry().getInt(RKEY_MULTIMON_START_MONITOR);

	if (startMonitor < gtkutil::MultiMonitor::getNumMonitors())
	{
		// Yes, connect the position tracker, this overrides the existing setting.
  		_windowPosition.connect(_window);
  		// Load the correct coordinates into the position tracker
		_windowPosition.fitToScreen(gtkutil::MultiMonitor::getMonitor(startMonitor), 0.8f, 0.8f);
		// Apply the position
		_windowPosition.applyPosition();
	}
	
	if (windowState & GDK_WINDOW_STATE_MAXIMIZED) {
		gtk_window_maximize(_window);
	}
	else {
		_windowPosition.connect(_window);
		_windowPosition.applyPosition();
	}
}

GtkWidget* MainFrame::createMenuBar()
{
	// Create the Filter menu entries before adding the menu bar
    FiltersMenu::addItemsToMainMenu();
    
    // Return the "main" menubar from the UIManager
    return GlobalUIManager().getMenuManager().get("main");
}

void MainFrame::create() {
	// Create the topmost window first
	createTopLevelWindow();

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(_window), vbox);
    gtk_widget_show(vbox);
    
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
	
	// Create the main container (this is a hbox)
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

	// Create the main container for layouts
	_mainContainer = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _mainContainer, TRUE, TRUE, 0);
    
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
    	GlobalEntityInspector().getWidget(), // page widget
    	_("Entity")
    );

	// Add the Media Browser page
	GlobalGroupDialog().addPage(
    	"mediabrowser",	// name
    	"Media", // tab title
    	"folder16.png", // tab icon 
    	MediaBrowser::getInstance().getWidget(), // page widget
    	_("Media")
    );
	
    // Add the console widget if using floating window mode, otherwise the
    // console is placed in the bottom-most split pane.
	GlobalGroupDialog().addPage(
    	"console",	// name
    	"Console", // tab title
    	"iconConsole16.png", // tab icon 
		Console::Instance().getWidget(), // page widget
    	_("Console")
    );

	// Load the previous window settings from the registry
	restoreWindowPosition();

	gtk_widget_show(GTK_WIDGET(_window));

	// Create the camera instance
	GlobalCamera().setParent(_window);

	// Start the autosave timer so that it can periodically check the map for changes 
	map::AutoSaver().startTimer();
  
	// Initialise the shaderclipboard
	GlobalShaderClipboard().clear();

	LayerControlDialog::init();
}

void MainFrame::saveWindowPosition() {
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
	GlobalRegistry().setAttribute(
		RKEY_WINDOW_STATE,
		"state",
		intToStr(gdk_window_get_state(GTK_WIDGET(_window)->window))
	);
}

void MainFrame::shutdown()
{
	// Shutdown the console
	Console::Instance().shutdown();

	// Shutdown the texturebrowser (before the GroupDialog gets shut down).
	GlobalTextureBrowser().destroyWindow();
	
	// Broadcast shutdown event to RadiantListeners
	radiant::getGlobalRadiant()->broadcastShutdownEvent();

	// Destroy the Overlay instance
	Overlay::destroyInstance();
	
	// Stop the AutoSaver class from being called
	map::AutoSaver().stopTimer();

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

void MainFrame::applyLayout(const std::string& name)
{
	if (getCurrentLayout() == name)
	{
		// nothing to do
		globalOutputStream() << "MainFrame: Won't activate layout " << name 
			<< ", is already active." << std::endl;
		return;
	}

	// Set or clear?
	if (!name.empty()) {
		// Try to find that new layout
		IMainFrameLayoutPtr layout = GlobalMainFrameLayoutManager().getLayout(name);

		if (layout == NULL) {
			globalErrorStream() << "MainFrame: Could not find layout with name " << name << std::endl;
			return;
		}

		// Found a new layout, remove the old one
		removeLayout();

		globalOutputStream() << "MainFrame: Activating layout " << name << std::endl;

		// Store and activate the new layout
		_currentLayout = layout;
		_currentLayout->activate();
	}
	else {
		// Empty layout name => remove
		removeLayout();
	}
}

std::string MainFrame::getCurrentLayout() {
	return (_currentLayout != NULL) ? _currentLayout->getName() : "";
}

// GTK callbacks
gboolean MainFrame::onDelete(GtkWidget* widget, GdkEvent* ev, MainFrame* self)
{
	if (GlobalMap().askForSave(_("Exit Radiant"))) {
		gtk_main_quit();
	}

	return TRUE;
}

// Define the static MainFrame module
module::StaticModule<MainFrame> mainFrameModule;

} // namespace ui
