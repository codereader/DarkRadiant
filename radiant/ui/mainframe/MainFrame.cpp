#include "MainFrame.h"

#include "i18n.h"
#include "RadiantModule.h"
#include "igroupdialog.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "ipreferencesystem.h"
#include "ientityinspector.h"
#include "iorthoview.h"
#include "iregistry.h"

#include "log/Console.h"
#include "xyview/GlobalXYWnd.h"
#include "camera/GlobalCamera.h"

#include "registry/registry.h"
#include "wxutil/MultiMonitor.h"

#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/mainframe/EmbeddedLayout.h"
#include "ui/mainframe/TopLevelFrame.h"
#include "map/Map.h"

#include "modulesystem/StaticModule.h"
#include <functional>
#include <fmt/format.h>

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
	_screenUpdatesEnabled(false) // not enabled until constructed
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
		_dependencies.insert(MODULE_ORTHOVIEWMANAGER);
		_dependencies.insert(MODULE_CAMERA);
	}

	return _dependencies;
}

void MainFrame::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "MainFrame::initialiseModule called." << std::endl;

	// Add another page for Multi-Monitor stuff
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Multi Monitor"));

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
			fmt::format("Monitor {0:d} ({1:d}x{2:d})", i, rect.GetWidth(), rect.GetHeight())
		);
	}

	page.appendCombo(_("Start DarkRadiant on monitor"), RKEY_MULTIMON_START_MONITOR, list);

	// Add the toggle max/min command for floating windows
	GlobalCommandSystem().addCommand("ToggleFullScreenCamera",
		std::bind(&MainFrame::toggleFullscreenCameraView, this, std::placeholders::_1)
	);
	GlobalEventManager().addCommand("ToggleFullScreenCamera", "ToggleFullScreenCamera");

	GlobalCommandSystem().addCommand("Exit", sigc::mem_fun(this, &MainFrame::exitCmd));
	GlobalEventManager().addCommand("Exit", "Exit");

#ifdef WIN32
	HMODULE lib = LoadLibrary(L"dwmapi.dll");

	if (lib != NULL)
	{
		void (WINAPI *dwmEnableComposition) (bool) =
			(void (WINAPI *) (bool)) GetProcAddress(lib, "DwmEnableComposition");

		if (dwmEnableComposition)
		{
			// Add a page for Desktop Composition stuff
			IPreferencePage& compatPage = GlobalPreferenceSystem().getPage(_("Settings/Compatibility"));

			compatPage.appendCheckBox(_("Disable Windows Desktop Composition"),
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

	disableScreenUpdates();
}

void MainFrame::exitCmd(const cmd::ArgumentList& args)
{
	// Just tell the main application window to close, which will invoke
	// appropriate event handlers.
	if (getWxTopLevelWindow() != nullptr)
	{
		getWxTopLevelWindow()->Close(false /* don't force */);
	}
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

#ifdef __linux__
	// #4526: In Linux, do another restore after the top level window has been shown
	// After startup, GTK emits onSizeAllocate events which trigger a Layout() sequence
	// messing up the pane positions, so do the restore() one more time after the main
	// window came up.
	_topLevelWindow->Bind(wxEVT_SHOW, [&](wxShowEvent& ev)
	{
		if (_currentLayout && ev.IsShown())
		{
			_currentLayout->restoreStateFromRegistry();
		}

		ev.Skip();
	});
#endif

	// register the commands
	GlobalMainFrameLayoutManager().registerCommands();

	enableScreenUpdates();

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

	// Load and sanitise the monitor number, the number of displays might have changed
	unsigned int startMonitor = registry::getValue<unsigned int>(RKEY_MULTIMON_START_MONITOR);

	if (startMonitor >= wxutil::MultiMonitor::getNumMonitors())
	{
		startMonitor = 0;
	}

	// Set up the size/position from registry or the defaults
	if (GlobalRegistry().keyExists(RKEY_WINDOW_STATE))
	{
		_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

		isMaximised = string::convert<bool>(GlobalRegistry().getAttribute(RKEY_WINDOW_STATE, "state"), true);
	}
	else
	{
		// If no state was found in the registry, fit the window into the start monitor rectangle
		_windowPosition.fitToScreen(wxutil::MultiMonitor::getMonitor(startMonitor), 0.8f, 0.8f);
	}

	// Connect the tracker which will also apply the stored size/position
	_windowPosition.connect(_topLevelWindow);

	if (isMaximised)
	{
		_topLevelWindow->Maximize(true);
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

    // Add the console
	IGroupDialog::PagePtr consolePage(new IGroupDialog::Page);

	consolePage->name = "console";
	consolePage->windowLabel = _("Console");
	consolePage->page = new Console(getWxTopLevelWindow());
	consolePage->tabIcon = "iconConsole16.png";
	consolePage->tabLabel = _("Console");
	consolePage->position = IGroupDialog::Page::Position::Console;

	GlobalGroupDialog().addPage(consolePage);

	// Load the previous window settings from the registry
	restoreWindowPosition();
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

		// Save the monitor number the window is currently displayed on
		registry::setValue(RKEY_MULTIMON_START_MONITOR,
			wxutil::MultiMonitor::getMonitorNumForWindow(_topLevelWindow));
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

void MainFrame::updateAllWindows(bool force)
{
	if (!_screenUpdatesEnabled) return;

    if (force)
    {
        GlobalCamera().forceDraw();
    }
    else
    {
        GlobalCamera().update();
    }

    GlobalXYWndManager().updateAllViews(force);
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
