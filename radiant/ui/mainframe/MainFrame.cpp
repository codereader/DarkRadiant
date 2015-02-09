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
#include "wxutil/MultiMonitor.h"

#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/mainframe/EmbeddedLayout.h"
#include "ui/mainframe/TopLevelFrame.h"

#include "modulesystem/StaticModule.h"
#include <functional>
#include <boost/format.hpp>

#include <wx/display.h>

#ifdef WIN32
#include <windows.h>
#endif

namespace 
{
	const std::string RKEY_WINDOW_LAYOUT = "user/ui/mainFrame/windowLayout";
	const std::string RKEY_WINDOW_STATE = "user/ui/mainFrame/window";
	const std::string RKEY_MULTIMON_START_MONITOR = "user/ui/multiMonitor/startMonitorNum";
	const std::string RKEY_DISABLE_WIN_DESKTOP_COMP = "user/ui/compatibility/disableWindowsDesktopComposition";

	const std::string RKEY_ACTIVE_LAYOUT = "user/ui/mainFrame/activeLayout";
}

namespace ui 
{

MainFrame::MainFrame() :
	_topLevelWindow(NULL),
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

	for (unsigned int i = 0; i < wxutil::MultiMonitor::getNumMonitors(); ++i)
	{
		wxRect rect = wxutil::MultiMonitor::getMonitor(i);

		list.push_back(
			(boost::format("Monitor %d (%dx%d)") % i % rect.GetWidth() % rect.GetHeight()).str()
		);
	}

	page->appendCombo(_("Start DarkRadiant on monitor"), RKEY_MULTIMON_START_MONITOR, list);

	// Add the toggle max/min command for floating windows
	GlobalCommandSystem().addCommand("ToggleFullScreenCamera",
		std::bind(&MainFrame::toggleFullscreenCameraView, this, std::placeholders::_1)
	);
	GlobalEventManager().addCommand("ToggleFullScreenCamera", "ToggleFullScreenCamera");

#ifdef WIN32
	HMODULE lib = LoadLibrary(L"dwmapi.dll");

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
	HMODULE lib = LoadLibrary(L"dwmapi.dll");

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

    updateAllWindows();
}

void MainFrame::removeLayout()
{
	// Sanity check
	if (_currentLayout == NULL) return;

	_currentLayout->deactivate();
	_currentLayout = IMainFrameLayoutPtr();
}

void MainFrame::preDestructionCleanup()
{
    // Unload the map (user has already been prompted to save, if appropriate)
    GlobalMap().freeMap();

	saveWindowPosition();

    // Free the layout
    if (_currentLayout != NULL)
    {
        removeLayout();
    }

	// Broadcast shutdown event to RadiantListeners
	radiant::getGlobalRadiant()->broadcastShutdownEvent();

	// Destroy the Overlay instance
	Overlay::destroyInstance();

	// Stop the AutoSaver class from being called
	map::AutoSaver().stopTimer();
}

void MainFrame::onTopLevelFrameClose(wxCloseEvent& ev)
{
    // If the event is vetoable, first check for unsaved data before closing
    if (ev.CanVeto() && !GlobalMap().askForSave(_("Exit DarkRadiant")))
    {
        // Do nothing
        ev.Veto();
    }
    else
    {
        wxASSERT(wxTheApp->GetTopWindow() == _topLevelWindow);

        _topLevelWindow->Hide();

        // Invoke cleanup code which still needs the GUI hierarchy to be
        // present
        preDestructionCleanup();

        // Destroy the actual window
        _topLevelWindow->Destroy();

        // wxWidgets is supposed to quit when the main window is destroyed, but
        // it doesn't so we need to exit the main loop manually. Probably we
        // are keeping some other window around internally which makes wx think
        // that the application is still needed.
        wxTheApp->ExitMainLoop();
    }
}

wxFrame* MainFrame::getWxTopLevelWindow()
{
	return _topLevelWindow;
}

wxBoxSizer* MainFrame::getWxMainContainer()
{
	return _topLevelWindow != NULL ? _topLevelWindow->getMainContainer() : NULL;
}

bool MainFrame::isActiveApp()
{
	return wxTheApp->IsActive();
}

void MainFrame::createTopLevelWindow()
{
	// Destroy any previous toplevel window
	if (_topLevelWindow)
	{
		_topLevelWindow->Destroy();
	}

	// Create a new window
	_topLevelWindow = new TopLevelFrame;
    wxTheApp->SetTopWindow(_topLevelWindow);

    // Listen for close events
    _topLevelWindow->Bind(wxEVT_CLOSE_WINDOW,
                          &MainFrame::onTopLevelFrameClose, this);
}

void MainFrame::restoreWindowPosition()
{
	// We start out maximised by default
	bool isMaximised = true;

	// Connect the window position tracker
	if (!GlobalRegistry().findXPath(RKEY_WINDOW_STATE).empty())
	{
		_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

		isMaximised = string::convert<bool>(GlobalRegistry().getAttribute(RKEY_WINDOW_STATE, "state"), true);
	}

	unsigned int startMonitor = registry::getValue<unsigned int>(RKEY_MULTIMON_START_MONITOR);

	if (startMonitor < wxDisplay::GetCount())
	{
		// Yes, connect the position tracker, this overrides the existing setting.
		_windowPosition.connect(_topLevelWindow);

  		// Load the correct coordinates into the position tracker
		_windowPosition.fitToScreen(wxDisplay(startMonitor).GetGeometry(), 0.8f, 0.8f);

		// Apply the position
		_windowPosition.applyPosition();
	}

	if (isMaximised)
	{
		_topLevelWindow->Maximize(true);
	}
	else
	{
		_windowPosition.connect(_topLevelWindow);
		_windowPosition.applyPosition();
	}
}

wxToolBar* MainFrame::getToolbar(IMainFrame::Toolbar type)
{
	return _topLevelWindow->getToolbar(type);
}

void MainFrame::create()
{
	// Create the topmost window first
	createTopLevelWindow();

	/* Construct the Group Dialog. This is the tabbed window that contains
     * a number of pages - usually Entities, Textures and possibly Console.
     */
    // Add entity inspector widget
	IGroupDialog::PagePtr entityInspectorPage(new IGroupDialog::Page);

	entityInspectorPage->name = "entity";
	entityInspectorPage->windowLabel = _("Entity");
	entityInspectorPage->page = GlobalEntityInspector().getWidget();
	entityInspectorPage->tabIcon = "cmenu_add_entity.png";
	entityInspectorPage->tabLabel = _("Entity");

	GlobalGroupDialog().addPage(entityInspectorPage);

	// Add the Media Browser page
	IGroupDialog::PagePtr mediaBrowserPage(new IGroupDialog::Page);

	mediaBrowserPage->name = "mediabrowser";
	mediaBrowserPage->windowLabel = _("Media");
	mediaBrowserPage->page = MediaBrowser::getInstance().getWidget();
	mediaBrowserPage->tabIcon = "folder16.png";
	mediaBrowserPage->tabLabel = _("Media");

	GlobalGroupDialog().addPage(mediaBrowserPage);

    // Add the console
	IGroupDialog::PagePtr consolePage(new IGroupDialog::Page);

	consolePage->name = "console";
	consolePage->windowLabel = _("Console");
	consolePage->page = new Console(getWxTopLevelWindow());
	consolePage->tabIcon = "iconConsole16.png";
	consolePage->tabLabel = _("Console");

	GlobalGroupDialog().addPage(consolePage);

	// Load the previous window settings from the registry
	restoreWindowPosition();

	_topLevelWindow->Show();

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

	if (_topLevelWindow)
	{
		GlobalRegistry().setAttribute(
			RKEY_WINDOW_STATE,
			"state",
			string::to_string(_topLevelWindow->IsMaximized())
		);
	}
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

// Define the static MainFrame module
module::StaticModule<MainFrame> mainFrameModule;

} // namespace ui
