#include "RadiantApp.h"

#include "i18n.h"
#include "iradiant.h"
#include "version.h"

#include "log/PIDFile.h"
#include "modulesystem/ModuleRegistry.h"
#include "module/CoreModule.h"

#include <wx/wxprec.h>
#include <wx/event.h>
#include <wx/cmdline.h>
#include <sigc++/functors/mem_fun.h>

#ifndef __linux__
#include "ui/splash/Splash.h"
#endif

#ifndef POSIX
#include "settings/LanguageManager.h"
#endif

#ifdef POSIX
#include <libintl.h>
#endif
#include <exception>
#include <iomanip>

#if defined (_DEBUG) && defined (WIN32) && defined (_MSC_VER)
#include "crtdbg.h"
#endif

// The startup event which will be queued in App::OnInit()
wxDEFINE_EVENT(EV_RadiantStartup, wxCommandEvent);

bool RadiantApp::OnInit()
{
	if (!wxApp::OnInit()) return false;

	// Initialise the debug flags
#if defined (_DEBUG) && defined (WIN32) && defined (_MSC_VER)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Stop wx's unhelpful debug messages about missing keyboard accel
	// strings from cluttering up the console
	wxLog::SetLogLevel(wxLOG_Warning);

	// Initialise the context (application path / settings path, is
	// OS-specific)
	_context.initialise(wxApp::argc, wxApp::argv);

	try
	{
		_coreModule.reset(new module::CoreModule(_context));

		auto* radiant = _coreModule->get();

		module::RegistryReference::Instance().setRegistry(radiant->getModuleRegistry());
		module::initialiseStreams(radiant->getLogWriter());
	}
	catch (module::CoreModule::FailureException & ex)
	{
		rError() << "Failed to load core module" << std::endl;
		throw ex;
	}

#ifndef POSIX
	// Initialise the language based on the settings in the user settings folder
	language::LanguageManager().init(_context);
#endif

#if defined(POSIX) && !defined(__APPLE__)
	// greebo: not sure if this is needed
	// Other POSIX gettext initialisation
	setlocale(LC_ALL, "");
	textdomain(GETTEXT_PACKAGE);
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
#endif

	// reset some locale settings back to standard c
	// this is e.g. needed for parsing float values from textfiles
	setlocale(LC_NUMERIC, "C");
	setlocale(LC_TIME, "C");

	wxInitAllImageHandlers();

	// Register to the start up signal
	Bind(EV_RadiantStartup, &RadiantApp::onStartupEvent, this);

	// Activate the Popup Error Handler
	_context.initErrorHandler();

	AddPendingEvent(wxCommandEvent(EV_RadiantStartup));

	return true;
}

int RadiantApp::OnExit()
{
	// Issue a shutdown() call to all the modules
	module::GlobalModuleRegistry().shutdownModules();

	_coreModule.reset();

	return wxApp::OnExit();
}

void RadiantApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	// Remove '/' as parameter starter to allow for "/path" style args
	parser.SetSwitchChars("-");

	parser.AddLongSwitch("disable-sound", _("Disable sound for this session."));
	parser.AddLongOption("verbose", _("Verbose logging."));

	parser.AddParam("Map file", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddParam("fs_game=<game>", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddParam("fs_game_base=<gamebase>", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
}

bool RadiantApp::OnExceptionInMainLoop()
{
	try
	{
		// This method is called by the main loop controlling code, 
		// from within the catch(...) block. Let's re-throw the current 
		// exception and catch it to print the error message at the very least.
		throw;
	}
	catch (const std::exception& ex)
	{
		rError() << "Unhandled Exception: " << ex.what() << std::endl;
	}

	return wxApp::OnExceptionInMainLoop();
}

void RadiantApp::onStartupEvent(wxCommandEvent& ev)
{
	// Create the radiant.pid file in the settings folder
	// (emits a warning if the file already exists (due to a previous startup failure))
	applog::PIDFile pidFile(PID_FILENAME);

#ifndef __linux__
	// We skip the splash screen in Linux, but the other platforms will show a progress bar
	// Connect the progress callback to the Splash instance.
	ui::Splash::OnAppStartup();
#endif

    try
    {
		module::GlobalModuleRegistry().loadAndInitialiseModules();
    }
    catch (const std::exception& e)
    {
        rConsole() << "Exception thrown while initialising ModuleRegistry: "
                   << e.what() << std::endl;
        abort();
    }

	// Scope ends here, PIDFile is deleted by its destructor
}
