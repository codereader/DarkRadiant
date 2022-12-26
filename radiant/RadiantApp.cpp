#include "RadiantApp.h"

#include "i18n.h"
#include "iradiant.h"
#include "version.h"

#include "log/PIDFile.h"
#include "module/CoreModule.h"
#include "messages/GameConfigNeededMessage.h"
#include "ui/prefdialog/GameSetupDialog.h"
#include "module/StaticModule.h"
#include "settings/LocalisationProvider.h"
#include "log/PopupErrorHandler.h"

#include <wx/wxprec.h>
#include <wx/event.h>
#include <wx/cmdline.h>
#include <wx/xrc/xmlres.h>
#include <sigc++/functors/mem_fun.h>

#ifndef __linux__
#include "ui/splash/Splash.h"
#endif

#ifdef POSIX
#include <libintl.h>
#endif
#include <exception>
#include <iomanip>

#if defined (_DEBUG) && defined (WIN32) && defined (_MSC_VER)
#include "crtdbg.h"
#endif

#if defined(__linux__)
// Function to intercept unwanted Gtk log messages on Linux
#include <glib.h>
GLogWriterOutput
log_black_hole(GLogLevelFlags, const GLogField*, gsize, gpointer)
{
    return G_LOG_WRITER_HANDLED;
}
#endif

// The startup event which will be queued in App::OnInit()
wxDEFINE_EVENT(EV_RadiantStartup, wxCommandEvent);

/**
 * Implements wxWidget's ArtProvider interface to allow custom stock item IDs for
 * bitmaps used in toolbars and other controls. The schema for these custom ArtIDs
 * is "darkradiant:filename.png" where filename.png is a file in DR's bitmap folder.
 * This schema is also valid when specified in XRC files.
 */
class RadiantApp::ArtProvider final: public wxArtProvider
{
    std::string _searchPath;

public:
    // Use an absolute file path to the list of search paths this provider is covering
    ArtProvider(const std::string& searchPath) :
        _searchPath(searchPath)
    {
        wxArtProvider::Push(this);
    }

    wxBitmap CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size) override
    {
        auto filename = id.ToStdString();
        const auto& prefix = ArtIdPrefix();

        // We listen only to "darkradiant" art IDs
        if (string::starts_with(filename, prefix))
        {
            auto filePath = _searchPath + filename.substr(prefix.length());

            if (os::fileOrDirExists(filePath))
            {
#ifdef __WXMSW__
                return wxBitmap(wxImage(filePath));
#else
                wxBitmap bm;
                if (bm.LoadFile(filePath)) {
                    return bm;
                }

                rError() << "Failed to load bitmap [" << filePath << "]\n";
#endif
            }
        }

        return wxNullBitmap;
    }

    static const std::string& ArtIdPrefix()
    {
        static std::string _artIdPrefix = "darkradiant:";
        return _artIdPrefix;
    }
};

RadiantApp::RadiantApp()
{}

RadiantApp::~RadiantApp()
{}

bool RadiantApp::OnInit()
{
	if (!wxApp::OnInit()) return false;

	// Initialise the debug flags
#if defined (_DEBUG) && defined (WIN32) && defined (_MSC_VER)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#if defined(__linux__)
    // Intercept and discard Gtk log messages emitted from wxGTK, which we
    // cannot control or fix, and which obliterate any attempt to use console
    // output for debugging (due to fresh Gtk-CRITICAL messages being emitted
    // several times per second)
    g_log_set_writer_func(log_black_hole, nullptr, nullptr);
#endif

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
	catch (module::CoreModule::FailureException& ex)
	{
		// Streams are not yet initialised, so log to std::err at this point
		std::cerr << ex.what() << std::endl;
		return false;
	}

	// Register the localisation helper before initialising the modules
	settings::LocalisationProvider::Initialise(_context);
	auto& languageManager = _coreModule->get()->getLanguageManager();
	languageManager.registerProvider(settings::LocalisationProvider::Instance());

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

    // Set up art provider, logging, XRC file handlers
    initWxWidgets();

	// Register to the start up signal
	Bind(EV_RadiantStartup, &RadiantApp::onStartupEvent, this);

	AddPendingEvent(wxCommandEvent(EV_RadiantStartup));

	return true;
}

void RadiantApp::initWxWidgets()
{
    // Stop wx's unhelpful debug messages about missing keyboard accel
    // strings from cluttering up the console
    wxLog::SetLogLevel(wxLOG_Warning);

    // On Linux, display wxWidgets log messages on stderr rather than popping up largely
    // useless (modal and often repeated) dialog boxes. TODO: find a better solution for
    // Windows as well (probably redirecting them to the in-application Console would be
    // best since stderr is not easily accessible).
#if defined(__linux__)
    wxLog::SetActiveTarget(new wxLogStderr() /* lifetime managed by wxWidgets */);
#endif

    wxFileSystem::AddHandler(new wxLocalFSHandler);
    wxXmlResource::Get()->InitAllHandlers();

    // Our XRC resource files are stored in the ui/ folder.
    wxXmlResource::Get()->Load(_context.getRuntimeDataPath() + "ui/*.xrc");

    // We only need PNG and JPEG for our local images. BMP is enabled by default.
#if wxUSE_LIBPNG
    wxImage::AddHandler(new wxPNGHandler);
#endif
#if wxUSE_LIBJPEG
    wxImage::AddHandler(new wxJPEGHandler);
#endif

    // Register the local art provider
    _bitmapArtProvider = std::make_unique<ArtProvider>(_context.getBitmapsPath());
}

void RadiantApp::cleanupWxWidgets()
{
    _bitmapArtProvider.reset();
    wxImage::CleanUpHandlers();
    wxXmlResource::Get()->ClearHandlers();
    wxFileSystem::CleanUpHandlers();
}

int RadiantApp::OnExit()
{
	// Issue a shutdown() call to all the modules
	module::GlobalModuleRegistry().shutdownModules();

	auto& languageManager = _coreModule->get()->getLanguageManager();
	languageManager.clearProvider();

	// Clean up static resources
	settings::LocalisationProvider::Cleanup();

	_coreModule.reset();

    cleanupWxWidgets();

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
        radiant::PopupErrorHandler::HandleError(_("Real Hard DarkRadiant Failure"), 
            std::string(ex.what()) + "\n\n" + _("Break into the debugger?"));
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

	// In first-startup scenarios the game configuration is not present
	// in which case the GameManager will dispatch a message asking 
	// for showing a dialog or similar. Connect the listener.
	_coreModule->get()->getMessageBus().addListener(radiant::IMessage::Type::GameConfigNeeded,
        radiant::TypeListener<game::ConfigurationNeeded>(ui::GameSetupDialog::HandleGameConfigMessage));
	
	// Pick up all the statically defined modules and register them
	module::internal::StaticModuleList::RegisterModules();

	// Register to the modules unloading event, we need to get notified
	// before the DLLs/SOs are relased to give wxWidgets a chance to clean up
	_modulesUnloadingHandler = _coreModule->get()->getModuleRegistry().signal_modulesUnloading()
		.connect(sigc::mem_fun(this, &RadiantApp::onModulesUnloading));

	try
	{
		// Startup the application
		_coreModule->get()->startup();
	}
	catch (const radiant::IRadiant::StartupFailure& ex)
	{
		// An unhandled exception during module initialisation => display a popup and exit
		rError() << "Unhandled Exception: " << ex.what() << std::endl;
		wxutil::Messagebox::ShowFatalError(ex.what(), nullptr);
	}

	// Scope ends here, PIDFile is deleted by its destructor
}

void RadiantApp::onModulesUnloading()
{
	// We need to delete all pending objects before unloading modules
	// wxWidgets needs a chance to delete them before memory access is denied
	if (wxTheApp != nullptr)
	{
		wxTheApp->ProcessIdle();
        wxTheApp->DeletePendingEvents();
	}

	_modulesUnloadingHandler.disconnect();
}
