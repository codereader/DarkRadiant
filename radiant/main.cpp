#include "main.h"

#include "i18n.h"
#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "imainframe.h"

#include "os/file.h"
#include "os/path.h"

#include "log/LogFile.h"
#include "log/PIDFile.h"
#include "log/LogStream.h"
#include "map/Map.h"
#include "settings/GameManager.h"
#include "ui/splash/Splash.h"
#include "RadiantModule.h"
#include "modulesystem/ModuleLoader.h"
#include "modulesystem/ModuleRegistry.h"
#include "Profile.h"

#ifndef POSIX
#include "settings/LanguageManager.h"
#endif

#include <gtkmm/main.h>
#include <gtkmm/gl/init.h>

#ifdef HAVE_GTKSOURCEVIEW
#include <gtksourceviewmm/init.h>
#endif

#include <exception>

#if defined (_DEBUG) && defined (WIN32) && defined (_MSC_VER)
#include "crtdbg.h"
#endif

void crt_init()
{
#if defined (_DEBUG) && defined (WIN32) && defined (_MSC_VER)
  _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
}

/**
 * Main entry point for the application.
 */
int main (int argc, char* argv[])
{
    // Initialise the debug flags
    crt_init();

    // Set the stream references for rMessage(), redirect std::cout, etc.
    applog::initialiseLogStreams();

    // Initialise the context (application path / settings path, is OS-specific)
    module::ModuleRegistry::Instance().initialiseContext(argc, argv);

    // Acquire the appplication context ref (shortcut)
    const ApplicationContext& ctx = module::getRegistry().getApplicationContext();

    // The settings path is set, start logging now
    applog::LogFile::create("darkradiant.log");

#ifndef POSIX
    // Initialise the language based on the settings in the user settings folder
    // This needs to happen before gtk_init() to set up the environment for GTK
    language::LanguageManager().init(ctx);
#endif

    // Initialise gtkmm (don't set locale on Windows)
#ifndef POSIX
    Gtk::Main gtkmm_main(argc, argv, false);
#else
    Gtk::Main gtkmm_main(argc, argv, true);

#ifndef LOCALEDIR
#error LOCALEDIR not defined
#endif

    // Other POSIX gettext initialisation
    setlocale(LC_ALL, "");
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);

#endif

	// Initialise Glib threading if necessary
	if (!Glib::thread_supported()) 
	{
		Glib::thread_init();
	}

    Glib::add_exception_handler(&std::terminate);

#ifdef HAVE_GTKSOURCEVIEW
    // Initialise gtksourceviewmm
    gtksourceview::init();
#endif

    // Initialise GTKGLExtmm
    Gtk::GL::init(argc, argv);

    // reset some locale settings back to standard c
    // this is e.g. needed for parsing float values from textfiles
    setlocale(LC_NUMERIC, "C");
    setlocale(LC_TIME, "C");

    // Now that GTK is ready, activate the Popup Error Handler
    module::ModuleRegistry::Instance().initErrorHandler();

    {
        // Create the radiant.pid file in the settings folder
        // (emits a warning if the file already exists (due to a previous startup failure))
        applog::PIDFile pidFile(PID_FILENAME);

        ui::Splash::Instance().show_all();

        // Initialise the Reference in the GlobalModuleRegistry() accessor.
        module::RegistryReference::Instance().setRegistry(module::getRegistry());

        ui::Splash::Instance().setProgressAndText(_("Searching for Modules"), 0.0f);

        // Invoke the ModuleLoad routine to load the DLLs from modules/ and plugins/
#if defined(POSIX) && defined(PKGLIBDIR)
        // Load modules from compiled-in path (e.g. /usr/lib/darkradiant)
        module::Loader::loadModules(PKGLIBDIR);
#else
        // Load modules from application-relative path
        module::Loader::loadModules(ctx.getApplicationPath());
#endif

        module::getRegistry().initialiseModules();

        radiant::getGlobalRadiant()->postModuleInitialisation();

        // Delete the splash screen here
        ui::Splash::Instance().destroy();

        // Scope ends here, PIDFile is deleted by its destructor
    }

    // greebo: Check if we should run an automated test
    if (!profile::CheckAutomatedTestRun())
	{
        // Start the GTK main loop. This will run until a quit command is given by
        // the user
        Gtk::Main::run();
    }

    GlobalMap().freeMap();

    GlobalMainFrame().destroy();

    // Issue a shutdown() call to all the modules
    module::GlobalModuleRegistry().shutdownModules();

    // Close the logfile
    applog::LogFile::close();
    applog::shutdownStreams();

    return EXIT_SUCCESS;
}

