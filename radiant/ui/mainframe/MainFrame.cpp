#include "MainFrame.h"

#include "i18n.h"
#include "RadiantModule.h"
#include "iuimanager.h"
#include "idialogmanager.h"
#include "igroupdialog.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"
#include "igrid.h"
#include "ientityinspector.h"
#include "iorthoview.h"

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

#include "registry/registry.h"
#include "map/AutoSaver.h"
#include "brush/BrushModule.h"
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
const std::string& MainFrame::getName() const
{
	static std::string _name(MODULE_MAINFRAME);
	return _name;
}

const StringSet& MainFrame::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MAINFRAME_LAYOUT_MANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_ORTHOVIEWMANAGER);
	}

	return _dependencies;
}

void MainFrame::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "MainFrame::initialiseModule called." << std::endl;

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
		Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitor(i);

		list.push_back(
			(boost::format("Monitor %d (%dx%d)") % i % rect.get_width() % rect.get_height()).str()
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

			GlobalRegistry().signalForKey(RKEY_DISABLE_WIN_DESKTOP_COMP).connect(
                sigc::mem_fun(this, &MainFrame::keyChanged)
            );
		}

		FreeLibrary(lib);
	}

	// Load the value and act
	setDesktopCompositionEnabled(!registry::getValue<bool>(RKEY_DISABLE_WIN_DESKTOP_COMP));
#endif
}

void MainFrame::shutdownModule()
{
	rMessage() << "MainFrame::shutdownModule called." << std::endl;
}

void MainFrame::keyChanged()
{
#ifdef WIN32
	setDesktopCompositionEnabled(!registry::getValue<bool>(RKEY_DISABLE_WIN_DESKTOP_COMP));
#endif
}

#ifdef WIN32

// Pulled those defs from Dwmapi.h
#define DWM_EC_DISABLECOMPOSITION         0
#define DWM_EC_ENABLECOMPOSITION          1

void MainFrame::setDesktopCompositionEnabled(bool enabled)
{
	HMODULE lib = LoadLibrary("dwmapi.dll");

	if (lib != NULL)
	{
		HRESULT (WINAPI *dwmEnableComposition) (UINT) =
			(HRESULT (WINAPI *) (UINT)) GetProcAddress(lib, "DwmEnableComposition");

		if (dwmEnableComposition)
		{
			HRESULT result = dwmEnableComposition(enabled ? DWM_EC_ENABLECOMPOSITION : DWM_EC_DISABLECOMPOSITION);

			if (!SUCCEEDED(result))
			{
				rError() << "Could not disable desktop composition" << std::endl;
			}
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

	if (activeLayout.empty())
	{
		activeLayout = EMBEDDED_LAYOUT_NAME; // fall back to hardcoded layout
	}

	// Apply the layout
	applyLayout(activeLayout);

	if (_currentLayout == NULL)
	{
		// Layout is still empty, this is not good
		rError() << "Could not restore layout " << activeLayout << std::endl;

		if (activeLayout != EMBEDDED_LAYOUT_NAME)
		{
			// Try to fallback to floating layout
			applyLayout(EMBEDDED_LAYOUT_NAME);
		}
	}

	// register the commands
	GlobalMainFrameLayoutManager().registerCommands();
}

void MainFrame::removeLayout()
{
	// Sanity check
	if (_currentLayout == NULL) return;

	_currentLayout->deactivate();
	_currentLayout = IMainFrameLayoutPtr();
}

void MainFrame::destroy()
{
	saveWindowPosition();

	// Free the layout
	if (_currentLayout != NULL)
	{
		removeLayout();
	}

	_window->hide(); // hide the Gtk::Window

	shutdown();

	_window.reset(); // destroy the window
}

const Glib::RefPtr<Gtk::Window>& MainFrame::getTopLevelWindow()
{
	return _window;
}

bool MainFrame::isActiveApp()
{
	// Iterate over all top-level windows and check if any of them has focus
	std::vector<Gtk::Window*> toplevels = Gtk::Window::list_toplevels();

	for (std::size_t i = 0; i < toplevels.size(); ++i)
	{
		if (toplevels[i]->property_is_active())
		{
			return true;
		}
	}

	return false;
}

Gtk::Container* MainFrame::getMainContainer()
{
	return _mainContainer;
}

void MainFrame::createTopLevelWindow()
{
	// Destroy any previous toplevel window
	if (_window)
	{
		GlobalEventManager().disconnect(_window->get_toplevel());

		_window->hide();
	}

	// Create a new window
	_window = Glib::RefPtr<Gtk::Window>(new Gtk::Window(Gtk::WINDOW_TOPLEVEL));

	// Tell the XYManager which window the xyviews should be transient for
	GlobalXYWnd().setGlobalParentWindow(getTopLevelWindow());

	// Set the splash window transient to this toplevel
	Splash::Instance().setTopLevelWindow(getTopLevelWindow());

#ifndef WIN32
	{
		// Set the default icon for non-Win32-systems
		// (Win32 builds use the one embedded in the exe)
		std::string icon = GlobalRegistry().get(RKEY_BITMAPS_PATH) +
  						   "darkradiant_icon_64x64.png";

		Gtk::Window::set_default_icon_from_file(icon);
	}
#endif

	// Signal setup
	_window->add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK | Gdk::FOCUS_CHANGE_MASK);

	_window->signal_delete_event().connect(sigc::mem_fun(*this, &MainFrame::onDeleteEvent));

	// Notify the event manager
	GlobalEventManager().connect(getTopLevelWindow()->get_toplevel());
	GlobalEventManager().connectAccelGroup(getTopLevelWindow());
}

void MainFrame::restoreWindowPosition()
{
	// We start out maximised by default
	int windowState = Gdk::WINDOW_STATE_MAXIMIZED;

	// Connect the window position tracker
	if (!GlobalRegistry().findXPath(RKEY_WINDOW_STATE).empty())
	{
		_windowPosition.loadFromPath(RKEY_WINDOW_STATE);
		windowState = string::convert<int>(
            GlobalRegistry().getAttribute(RKEY_WINDOW_STATE, "state")
        );
	}

	int startMonitor = registry::getValue<int>(RKEY_MULTIMON_START_MONITOR);

	if (startMonitor < gtkutil::MultiMonitor::getNumMonitors())
	{
		// Yes, connect the position tracker, this overrides the existing setting.
		_windowPosition.connect(static_cast<Gtk::Window*>(_window->get_toplevel()));
  		// Load the correct coordinates into the position tracker
		_windowPosition.fitToScreen(gtkutil::MultiMonitor::getMonitor(startMonitor), 0.8f, 0.8f);
		// Apply the position
		_windowPosition.applyPosition();
	}

	if (windowState & Gdk::WINDOW_STATE_MAXIMIZED)
	{
		_window->maximize();
	}
	else
	{
		_windowPosition.connect(static_cast<Gtk::Window*>(_window->get_toplevel()));
		_windowPosition.applyPosition();
	}
}

Gtk::Widget* MainFrame::createMenuBar()
{
	// Create the Filter menu entries before adding the menu bar
    FiltersMenu::addItemsToMainMenu();

    // Return the "main" menubar from the UIManager
	return GlobalUIManager().getMenuManager().get("main");
}

Gtk::Toolbar* MainFrame::getToolbar(IMainFrame::Toolbar type)
{
	ToolbarMap::const_iterator found = _toolbars.find(type);

	return (found != _toolbars.end()) ? found->second : NULL;
}

void MainFrame::create()
{
	// Create the topmost window first
	createTopLevelWindow();

	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 0));
	_window->add(*vbox);

    vbox->show();

    // Retrieve the "main" menubar from the UIManager
	vbox->pack_start(*createMenuBar(), false, false, 0);

    // Instantiate the ToolbarManager and retrieve the view toolbar widget
	IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();

	Gtk::Toolbar* viewToolbar = tbCreator.getToolbar("view");

	if (viewToolbar != NULL)
	{
		_toolbars[TOOLBAR_HORIZONTAL] = viewToolbar;

		// Pack it into the main window
		_toolbars[TOOLBAR_HORIZONTAL]->show();

		vbox->pack_start(*_toolbars[TOOLBAR_HORIZONTAL], false, false, 0);
	}
	else
	{
		rWarning() << "MainFrame: Cannot instantiate view toolbar!" << std::endl;
	}

	// Create the main container (this is a hbox)
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 0));

	hbox->show();
	vbox->pack_start(*hbox, true, true, 0);

    // Get the edit toolbar widget
	Gtk::Toolbar* editToolbar = tbCreator.getToolbar("edit");

	if (editToolbar != NULL)
	{
		_toolbars[TOOLBAR_VERTICAL] = editToolbar;

		// Pack it into the main window
		_toolbars[TOOLBAR_VERTICAL]->show();

		hbox->pack_start(*_toolbars[TOOLBAR_VERTICAL], false, false, 0);
	}
	else
	{
		rWarning() << "MainFrame: Cannot instantiate edit toolbar!" << std::endl;
	}

	// Create the main container for layouts
	_mainContainer = Gtk::manage(new Gtk::VBox(false, 0));
	hbox->pack_start(*_mainContainer, true, true, 0);

    // Create and pack main statusbar
	Gtk::Widget* statusBar = GlobalUIManager().getStatusBarManager().getStatusBar();

	vbox->pack_end(*statusBar, false, false, 2);
	statusBar->show_all();

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
    	*MediaBrowser::getInstance().getWidget(), // page widget
    	_("Media")
    );

    // Add the console widget if using floating window mode, otherwise the
    // console is placed in the bottom-most split pane.
	GlobalGroupDialog().addPage(
    	"console",	// name
    	"Console", // tab title
    	"iconConsole16.png", // tab icon
		Console::Instance(), // page widget
    	_("Console")
    );

	// Load the previous window settings from the registry
	restoreWindowPosition();

	_window->show();

	// Create the camera instance
	GlobalCamera().setParent(getTopLevelWindow());

	// Start the autosave timer so that it can periodically check the map for changes
	map::AutoSaver().startTimer();

	// Initialise the shaderclipboard
	GlobalShaderClipboard().clear();

	LayerControlDialog::init();
}

void MainFrame::saveWindowPosition()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	Glib::RefPtr<Gdk::Window> window = _window->get_window();

	if (window)
	{
		GlobalRegistry().setAttribute(
			RKEY_WINDOW_STATE,
			"state",
			string::to_string(window->get_state())
		);
	}
}

void MainFrame::shutdown()
{
	// Shutdown and destroy the console
	Console::Instance().destroy();

	// Shutdown the texturebrowser (before the GroupDialog gets shut down).
	GlobalTextureBrowser().destroyWindow();

	// Broadcast shutdown event to RadiantListeners
	radiant::getGlobalRadiant()->broadcastShutdownEvent();

	// Destroy the Overlay instance
	Overlay::destroyInstance();

	// Stop the AutoSaver class from being called
	map::AutoSaver().stopTimer();
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

void MainFrame::updateAllWindows()
{
	GlobalCamera().update();
	GlobalXYWndManager().updateAllViews();
}

void MainFrame::setActiveLayoutName(const std::string& name)
{
    GlobalRegistry().set(RKEY_ACTIVE_LAYOUT, name);
}

void MainFrame::applyLayout(const std::string& name)
{
	if (getCurrentLayout() == name)
	{
		// nothing to do
		rMessage() << "MainFrame: Won't activate layout " << name
			<< ", is already active." << std::endl;
		return;
	}

	// Set or clear?
	if (!name.empty())
    {
		// Try to find that new layout
		IMainFrameLayoutPtr layout = GlobalMainFrameLayoutManager().getLayout(name);

		if (layout == NULL) {
			rError() << "MainFrame: Could not find layout with name " << name << std::endl;
			return;
		}

		// Found a new layout, remove the old one
		removeLayout();

		rMessage() << "MainFrame: Activating layout " << name << std::endl;

		// Store and activate the new layout
		_currentLayout = layout;
		_currentLayout->activate();
	}
	else {
		// Empty layout name => remove
		removeLayout();
	}
}

std::string MainFrame::getCurrentLayout()
{
	return (_currentLayout != NULL) ? _currentLayout->getName() : "";
}

IScopedScreenUpdateBlockerPtr MainFrame::getScopedScreenUpdateBlocker(const std::string& title, 
		const std::string& message, bool forceDisplay)
{
	return IScopedScreenUpdateBlockerPtr(new ScreenUpdateBlocker(title, message, forceDisplay));
}

// GTKmm callbacks
bool MainFrame::onDeleteEvent(GdkEventAny* ev)
{
	if (GlobalMap().askForSave(_("Exit Radiant")))
	{
		Gtk::Main::quit();
	}

	return true; // don't propagate
}

// Define the static MainFrame module
module::StaticModule<MainFrame> mainFrameModule;

} // namespace ui
