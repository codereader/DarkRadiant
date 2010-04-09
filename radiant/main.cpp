/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "main.h"

#include "i18n.h"
#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "imainframe.h"

#include <gtk/gtkmain.h>
#include <gtk/gtkgl.h>

#include "os/file.h"
#include "os/path.h"

#include "log/LogFile.h"
#include "log/PIDFile.h"
#include "log/LogStream.h"
#include "map/Map.h"
#include "mainframe_old.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "settings/GameManager.h"
#include "ui/splash/Splash.h"
#include "modulesystem/ModuleLoader.h"
#include "modulesystem/ModuleRegistry.h"

#ifdef _PROFILE
#include "Profile.h"
#endif

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
int main (int argc, char* argv[]) {
	
	// Initialise the debug flags
	crt_init();

	// Set the stream references for globalOutputStream(), redirect std::cout, etc.
	applog::initialiseLogStreams();

	// Initialise the context (application path / settings path, is OS-specific)
	module::ModuleRegistry::Instance().initialiseContext(argc, argv);

	// Acquire the appplication context ref (shortcut)
	const ApplicationContext& ctx = module::getRegistry().getApplicationContext();

	// The settings path is set, start logging now
	applog::LogFile::create("darkradiant.log");

	g_setenv("LANG", "en_US", TRUE);

	bindtextdomain(GETTEXT_PACKAGE, (ctx.getApplicationPath() + "i18n").c_str());
    // set encoding to utf-8 to prevent errors for Windows
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	
	// Initialise GTK
	gtk_disable_setlocale();
	gtk_init(&argc, &argv);

    // Initialise GTKGLExt
    gtk_gl_init(&argc, &argv);

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

		ui::Splash::Instance().show();
	
		// Initialise the Reference in the GlobalModuleRegistry() accessor. 
		module::RegistryReference::Instance().setRegistry(module::getRegistry());
	
		ui::Splash::Instance().setProgressAndText("Searching for Modules", 0.0f);
	
		// Invoke the ModuleLoad routine to load the DLLs from modules/ and plugins/
#if defined(POSIX) && defined(PKGLIBDIR)
        // Load modules from compiled-in path (e.g. /usr/lib/darkradiant)
        module::Loader::loadModules(PKGLIBDIR);
#else
        // Load modules from application-relative path
		module::Loader::loadModules(ctx.getApplicationPath());
#endif
	
		module::getRegistry().initialiseModules();
	
		ui::Splash::Instance().setProgressAndText("Creating PrefDialog", 0.85f);

		Radiant_Initialise();
		
		// Initialise the mediabrowser
		ui::Splash::Instance().setProgressAndText("Initialising MediaBrowser", 0.92f);
		ui::MediaBrowser::init();

		ui::Splash::Instance().setProgressAndText("Starting MainFrame", 0.95f);

		// Initialise the mainframe
		GlobalMainFrame().construct();

		// Load the shortcuts from the registry
   		GlobalEventManager().loadAccelerators();
	   	
   		// Update all accelerators, at this point all commands should be setup
   		GlobalUIManager().getMenuManager().updateAccelerators();
	  
		ui::Splash::Instance().setProgressAndText("Complete", 1.0f);  

		ui::Splash::Instance().hide();

		// Scope ends here, PIDFile is deleted by its destructor
	}

#ifdef _PROFILE
	// greebo: In profile builds, check if we should run an automated test
	if (!profile::CheckAutomatedTestRun()) {
		// Start the GTK main loop. This will run until a quit command is given by
		// the user
		gtk_main();
	}
#else 
	// Start the GTK main loop. This will run until a quit command is given by
	// the user
	gtk_main();
#endif
	
	GlobalMap().freeMap();

	GlobalMainFrame().destroy();

	// Issue a shutdown() call to all the modules
  	module::GlobalModuleRegistry().shutdownModules();

	// Close the logfile 
	applog::LogFile::close();
	applog::shutdownStreams();

	return EXIT_SUCCESS;
}

