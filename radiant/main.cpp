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

#include "version.h"

#include "debugging/debugging.h"

#include "iundo.h"
#include "imodule.h"
#include "iuimanager.h"
#include "ifilesystem.h"
#include "iregistry.h"
#include "ieventmanager.h"

#include <gtk/gtkmain.h>

#include "cmdlib.h"
#include "os/file.h"
#include "os/path.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

#include "gtkutil/messagebox.h"
#include "console.h"
#include "map/Map.h"
#include "mainframe.h"
#include "settings/PreferenceSystem.h"
#include "referencecache.h"
#include "ui/mru/MRU.h"
#include "settings/GameManager.h"
#include "ui/splash/Splash.h"
#include "modulesystem/ModuleLoader.h"
#include "modulesystem/ModuleRegistry.h"

#ifdef _PROFILE
#include "Profile.h"
#endif

#include <iostream>

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

class LineLimitedTextOutputStream : public TextOutputStream
{
  TextOutputStream& outputStream;
  std::size_t count;
public:
  LineLimitedTextOutputStream(TextOutputStream& outputStream, std::size_t count)
    : outputStream(outputStream), count(count)
  {
  }
  std::size_t write(const char* buffer, std::size_t length)
  {
    if(count != 0)
    {
      const char* p = buffer;
      const char* end = buffer+length;
      for(;;)
      {
        p = std::find(p, end, '\n');
        if(p == end)
        {
          break;
        }
        ++p;
        if(--count == 0)
        {
          length = p - buffer;
          break;
        }
      }
      outputStream.write(buffer, length);
    }
    return length;
  }
};

class PopupDebugMessageHandler : public DebugMessageHandler
{
  StringOutputStream m_buffer;
  Lock m_lock;
public:
  TextOutputStream& getOutputStream()
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
    LineLimitedTextOutputStream outputStream(getOutputStream(), 24);
    //write_stack_trace(outputStream);
    getOutputStream() << "----------------\n";
    globalErrorStream() << m_buffer.c_str();
    if(!m_lock.locked())
    {
      ScopedLock lock(m_lock);
#if defined _DEBUG
      m_buffer << "Break into the debugger?\n";
      bool handled = gtk_MessageBox(0, m_buffer.c_str(), "Radiant - Runtime Error", eMB_YESNO, eMB_ICONERROR) == eIDNO;
      m_buffer.clear();
      return handled;
#else
      m_buffer << "Please report this error to the developers\n";
      gtk_MessageBox(0, m_buffer.c_str(), "Radiant - Runtime Error", eMB_OK, eMB_ICONERROR);
      m_buffer.clear();
#endif
    }
    return true;
  }
};

typedef Static<PopupDebugMessageHandler> GlobalPopupDebugMessageHandler;

void streams_init()
{
  GlobalErrorStream::instance().setOutputStream(getSysPrintErrorStream());
  GlobalOutputStream::instance().setOutputStream(getSysPrintOutputStream());
}

void createPIDFile(const std::string& name) {
	std::string pidFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + name;

	FILE *pid;
	pid = fopen(pidFile.c_str(), "r");
	
	// Check for an existing radiant.pid file
	if (pid != NULL) {
		fclose (pid);

		if (remove (pidFile.c_str()) == -1) {
			std::string msg = "WARNING: Could not delete " + pidFile;
			gtk_MessageBox(0, msg.c_str(), "Radiant", eMB_OK, eMB_ICONERROR );
		}

	    std::string msg("Radiant failed to start properly the last time it was run.\n");
	    msg += "The failure may be related to invalid preference settings.\n";
		msg += "Do you want to rename your local user.xml file and restore the default settings?";

		if (gtk_MessageBox(0, msg.c_str(), "Radiant - Startup Failure", 
			   eMB_YESNO, eMB_ICONQUESTION) == eIDYES) 
		{
			resetPreferences();
		}
	}

	// create a primary .pid for global init run
	pid = fopen (pidFile.c_str(), "w");
	if (pid) {
		fclose (pid);
	}
}

void removePIDFile(const std::string& name) {
	std::string pidFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + name;

	// close the primary
	if (remove(pidFile.c_str()) == -1) {
		std::string msg = "WARNING: Could not delete " + pidFile;
		gtk_MessageBox(0, msg.c_str(), "Radiant", eMB_OK, eMB_ICONERROR );
	}
}

/**
 * Main entry point for the application.
 */
int main (int argc, char* argv[]) {
	
	// Initialise the debug flags
	crt_init();

	// Set the stream references for globalOutputStream() etc.
	streams_init();

	// Initialse the context (application path / settings path, is OS-specific)
	module::ModuleRegistry::Instance().initialiseContext(argc, argv);
	
	// Initialise GTK
	gtk_disable_setlocale();
	gtk_init(&argc, &argv);

	GlobalDebugMessageHandler::instance().setHandler(GlobalPopupDebugMessageHandler::instance());

	ui::Splash::Instance().show();
	
	// Initialise the Reference in the GlobalModuleRegistry() accessor. 
	module::RegistryReference::Instance().setRegistry(module::getRegistry());
	
	ui::Splash::Instance().setProgressAndText("Searching for Modules", 0.0f);
	
	// Invoke the ModuleLoad routine to load the DLLs from modules/ and plugins/
	const ApplicationContext& ctx = module::getRegistry().getApplicationContext();
	module::Loader::loadModules(ctx.getApplicationPath());
	
	module::getRegistry().initialiseModules();
	
	// Create the radiant.pid file in the settings folder 
	// (emits a warning if the file already exists (due to a previous startup failure)) 
	createPIDFile("radiant.pid");
	
	ui::Splash::Instance().setProgressAndText("Creating Logfile", 0.77f);

	// The settings path is set, start logging now
	Sys_LogFile(true);
	
	ui::Splash::Instance().setProgressAndText("Creating PrefDialog", 0.79f);

	// The VFS is setup at this point, we can load the modules
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

	if (GlobalMRU().loadLastMap() && GlobalMRU().getLastMapName() != "") {
		GlobalMap().load(GlobalMRU().getLastMapName());
	}
	else {
		GlobalMap().createNew();
	}

	// Remove the radiant.pid file again after loading all the settings
	removePIDFile("radiant.pid");

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

  // close the log file if any
  Sys_LogFile(false);

  return EXIT_SUCCESS;
}

