#include "MainFrame.h"

#include "i18n.h"
#include "ui/igroupdialog.h"
#include "imap.h"
#include "ui/ieventmanager.h"
#include "ipreferencesystem.h"
#include "ui/ientityinspector.h"
#include "ieclass.h"
#include "iorthoview.h"
#include "iregistry.h"
#include "iradiant.h"

#include "log/Console.h"
#include "xyview/GlobalXYWnd.h"
#include "camera/CameraWndManager.h"

#include "registry/registry.h"
#include "wxutil/MultiMonitor.h"

#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/mainframe/AuiLayout.h"
#include "ui/mainframe/TopLevelFrame.h"
#include "textool/TexTool.h"

#include "module/StaticModule.h"
#include "messages/ApplicationShutdownRequest.h"
#include <functional>
#include <fmt/format.h>
#include <sigc++/functors/mem_fun.h>

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
	_screenUpdatesEnabled(false), // not enabled until constructed
    _defLoadingBlocksUpdates(false)
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
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_ORTHOVIEWMANAGER);
		_dependencies.insert(MODULE_ECLASSMANAGER);
		_dependencies.insert(MODULE_MAP);
	}

	return _dependencies;
}

void MainFrame::initialiseModule(const IApplicationContext& ctx)
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

	GlobalCommandSystem().addCommand("Exit", sigc::mem_fun(this, &MainFrame::exitCmd));

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

	_mapNameChangedConn = GlobalMapModule().signal_mapNameChanged().connect(
		sigc::mem_fun(this, &MainFrame::updateTitle)
	);

	_mapModifiedChangedConn = GlobalMapModule().signal_modifiedChanged().connect(
		sigc::mem_fun(this, &MainFrame::updateTitle)
	);

    // When the eclass defs are in progress of being loaded, block all updates
    GlobalEntityClassManager().defsLoadingSignal().connect(
        [this]() { _defLoadingBlocksUpdates = true; }
    );

    GlobalEntityClassManager().defsLoadedSignal().connect([this]()
        {
            _defLoadingBlocksUpdates = false;
        }
    );

	// Subscribe for the post-module init event
	module::GlobalModuleRegistry().signal_allModulesInitialised().connect(
		sigc::mem_fun(this, &MainFrame::postModuleInitialisation));
}

void MainFrame::shutdownModule()
{
	rMessage() << "MainFrame::shutdownModule called." << std::endl;

	_mapNameChangedConn.disconnect();
	_mapModifiedChangedConn.disconnect();

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

void MainFrame::postModuleInitialisation()
{
	// Initialise the mainframe
	construct();

	// Load the shortcuts from the registry
	GlobalEventManager().loadAccelerators();

	// Show the top level window as late as possible
	getWxTopLevelWindow()->Show();

	signal_MainFrameReady().emit();
	signal_MainFrameReady().clear();
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
		activeLayout = AUI_LAYOUT_NAME; // fall back to hardcoded layout
	}

	// Apply the layout
	applyLayout(activeLayout);

	if (_currentLayout == NULL)
	{
		// Layout is still empty, this is not good
		rError() << "Could not restore layout " << activeLayout << std::endl;

		if (activeLayout != AUI_LAYOUT_NAME)
		{
			// Try to fallback to floating layout
			applyLayout(AUI_LAYOUT_NAME);
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

	// Emit the "constructed" signal to give modules a chance to register
	// their UI parts. Clear the signal afterwards.
	signal_MainFrameConstructed().emit();
	signal_MainFrameConstructed().clear();

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
	saveWindowPosition();

    // Free the layout
    if (_currentLayout)
    {
        removeLayout();
    }
}

void MainFrame::updateTitle()
{
	if (_topLevelWindow == nullptr)
	{
		return;
	}

	std::string title = GlobalMapModule().getMapName();

	if (GlobalMapModule().isModified())
	{
		title += " *";
	}

	_topLevelWindow->SetTitle(title);
}

void MainFrame::onTopLevelFrameClose(wxCloseEvent& ev)
{
    // If the event is vetoable, issue the shutdown message and get the green light
    if (ev.CanVeto())
    {
		radiant::ApplicationShutdownRequest request;
		GlobalRadiantCore().getMessageBus().sendMessage(request);

		if (request.isDenied())
		{
			// Keep running
			ev.Veto();
			return;
		}
    }

    wxASSERT(wxTheApp->GetTopWindow() == _topLevelWindow);

    _topLevelWindow->Hide();

    // Invoke cleanup code which still needs the GUI hierarchy to be
    // present
    preDestructionCleanup();

	// Broadcast shutdown event
	signal_MainFrameShuttingDown().emit();
	signal_MainFrameShuttingDown().clear();

    // Destroy the actual window
    _topLevelWindow->Destroy();
    _topLevelWindow = nullptr;

    // wxWidgets is supposed to quit when the main window is destroyed, but
    // it doesn't so we need to exit the main loop manually. Probably we
    // are keeping some other window around internally which makes wx think
    // that the application is still needed.
    wxTheApp->ExitMainLoop();
}

wxFrame* MainFrame::getWxTopLevelWindow()
{
	return _topLevelWindow;
}

wxBoxSizer* MainFrame::getWxMainContainer()
{
	return _topLevelWindow != nullptr ? _topLevelWindow->getMainContainer() : nullptr;
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
    // Pass any request for the camera view toolbar to the active CamWnd, if
    // any
    if (type == Toolbar::CAMERA)
    {
        CamWndPtr cw = GlobalCamera().getActiveCamWnd();
        return cw ? cw->getToolbar() : nullptr;
    }

    // Main window toolbar
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

bool MainFrame::screenUpdatesEnabled()
{
	return _screenUpdatesEnabled && !_defLoadingBlocksUpdates;
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

    if (TexTool::Instance().IsShown())
    {
        if (force)
        {
            TexTool::Instance().forceRedraw();
        }
        else
        {
            TexTool::Instance().queueDraw();
        }
    }
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

sigc::signal<void>& MainFrame::signal_MainFrameConstructed()
{
	return _sigMainFrameConstructed;
}

sigc::signal<void>& MainFrame::signal_MainFrameReady()
{
	return _sigMainFrameReady;
}

sigc::signal<void>& MainFrame::signal_MainFrameShuttingDown()
{
	return _sigMainFrameShuttingDown;
}

// Define the static MainFrame module
module::StaticModuleRegistration<MainFrame> mainFrameModule;

} // namespace ui
