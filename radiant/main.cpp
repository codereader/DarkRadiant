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

/*! \mainpage GtkRadiant Documentation Index

\section intro_sec Introduction

This documentation is generated from comments in the source code.

\section links_sec Useful Links

\link include/itextstream.h include/itextstream.h \endlink - Global output and error message streams, similar to std::cout and std::cerr. \n

FileInputStream - similar to std::ifstream (binary mode) \n
FileOutputStream - similar to std::ofstream (binary mode) \n
TextFileInputStream - similar to std::ifstream (text mode) \n
TextFileOutputStream - similar to std::ofstream (text mode) \n
StringOutputStream - similar to std::stringstream \n

\link string/string.h string/string.h \endlink - C-style string comparison and memory management. \n
\link os/path.h os/path.h \endlink - Path manipulation for radiant's standard path format \n
\link os/file.h os/file.h \endlink - OS file-system access. \n

Array - automatic array memory management \n

\link math/matrix.h math/matrix.h \endlink - Matrices \n
\link math/quaternion.h math/quaternion.h \endlink - Quaternions \n
\link math/plane.h math/plane.h \endlink - Planes \n
\link math/aabb.h math/aabb.h \endlink - AABBs \n

Callback MemberCaller FunctionCaller - callbacks similar to using boost::function with boost::bind \n

\link generic/bitfield.h generic/bitfield.h \endlink - Type-safe bitfield \n
\link generic/enumeration.h generic/enumeration.h \endlink - Type-safe enumeration \n

DefaultAllocator - Memory allocation using new/delete, compliant with std::allocator interface \n

\link debugging/debugging.h debugging/debugging.h \endlink - Debugging macros \n

*/

#include "main.h"

#include "iregistry.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "debugging/debugging.h"

#include <gtk/gtkmain.h>

#include "os/file.h"
#include "os/path.h"

#include "log/LogFile.h"
#include "log/PIDFile.h"
#include "log/LogStream.h"
#include "map/Map.h"
#include "mainframe.h"
#include "ui/mru/MRU.h"
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

class Lock
{
  bool m_locked;
public:
  Lock() : m_locked(false)
  {
  }
  void lock()
  {
    m_locked = true;
  }
  void unlock()
  {
    m_locked = false;
  }
  bool locked() const
  {
    return m_locked;
  }
};

class ScopedLock
{
  Lock& m_lock;
public:
  ScopedLock(Lock& lock) : m_lock(lock)
  {
    m_lock.lock();
  }
  ~ScopedLock()
  {
    m_lock.unlock();
  }
};

class PopupDebugMessageHandler : public DebugMessageHandler
{
  std::ostringstream m_buffer;
  Lock m_lock;
public:
  std::ostream& getOutputStream()
  {
    if(!m_lock.locked())
    {
      return m_buffer;
    }
    return globalErrorStream();
  }
  bool handleMessage()
  {
    getOutputStream() << "----------------\n";
    globalErrorStream() << m_buffer.str();
    if(!m_lock.locked())
    {
      ScopedLock lock(m_lock);
#if defined _DEBUG
      m_buffer << "Break into the debugger?\n";
      bool handled = gtk_MessageBox(0, m_buffer.str().c_str(), "Radiant - Runtime Error", eMB_YESNO, eMB_ICONERROR) == eIDNO;
      m_buffer.clear();
      return handled;
#else
      m_buffer << "Please report this error to the developers\n";
      gtk_MessageBox(0, m_buffer.str().c_str(), "Radiant - Runtime Error", eMB_OK, eMB_ICONERROR);
      m_buffer.clear();
#endif
    }
    return true;
  }
};

typedef Static<PopupDebugMessageHandler> GlobalPopupDebugMessageHandler;

/**
 * Main entry point for the application.
 */
int main (int argc, char* argv[]) {
	
	// Initialise the debug flags
	crt_init();

	// Set the stream references for globalOutputStream(), redirect std::cout, etc.
	applog::initialiseLogStreams();

	// Initialse the context (application path / settings path, is OS-specific)
	module::ModuleRegistry::Instance().initialiseContext(argc, argv);

	// The settings path is set, start logging now
	applog::LogFile::create("darkradiant.log");
	
	// Initialise GTK
	gtk_disable_setlocale();
	gtk_init(&argc, &argv);

	{
		// Create the radiant.pid file in the settings folder 
		// (emits a warning if the file already exists (due to a previous startup failure)) 
		applog::PIDFile pidFile(PID_FILENAME);

		GlobalDebugMessageHandler::instance().setHandler(GlobalPopupDebugMessageHandler::instance());

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
		const ApplicationContext& ctx = module::getRegistry().getApplicationContext();
		module::Loader::loadModules(ctx.getApplicationPath());
#endif
	
		module::getRegistry().initialiseModules();
	
		ui::Splash::Instance().setProgressAndText("Creating PrefDialog", 0.85f);

		Radiant_Initialise();
		
		ui::Splash::Instance().setProgressAndText("Starting MainFrame", 0.92f);
		
		g_pParentWnd = 0;
	  g_pParentWnd = new MainFrame();
	  
	  // Load the shortcuts from the registry
   		GlobalEventManager().loadAccelerators();
	   	
   		// Update all accelerators, at this point all commands should be setup
   		GlobalUIManager().getMenuManager().updateAccelerators();
	  
		ui::Splash::Instance().setProgressAndText("Complete", 1.0f);  

		ui::Splash::Instance().hide();

		std::string lastMap = GlobalMRU().getLastMapName();
		if (GlobalMRU().loadLastMap() && !lastMap.empty() && file_exists(lastMap.c_str())) {
			GlobalMap().load(lastMap);
		}
		else {
			GlobalMap().createNew();
		}

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

  delete g_pParentWnd;

	GlobalMRU().saveRecentFiles();

  	// Issue a shutdown() call to all the modules
  	module::GlobalModuleRegistry().shutdownModules();

	// Close the logfile 
	applog::LogFile::close();
	applog::shutdownStreams();

	return EXIT_SUCCESS;
}

