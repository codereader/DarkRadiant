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

::CopiedString - automatic string memory management \n
Array - automatic array memory management \n
HashTable - generic hashtable, similar to std::hash_map \n

\link math/matrix.h math/matrix.h \endlink - Matrices \n
\link math/quaternion.h math/quaternion.h \endlink - Quaternions \n
\link math/plane.h math/plane.h \endlink - Planes \n
\link math/aabb.h math/aabb.h \endlink - AABBs \n

Callback MemberCaller FunctionCaller - callbacks similar to using boost::function with boost::bind \n
SmartPointer SmartReference - smart-pointer and smart-reference similar to Loki's SmartPtr \n

\link generic/bitfield.h generic/bitfield.h \endlink - Type-safe bitfield \n
\link generic/enumeration.h generic/enumeration.h \endlink - Type-safe enumeration \n

DefaultAllocator - Memory allocation using new/delete, compliant with std::allocator interface \n

\link debugging/debugging.h debugging/debugging.h \endlink - Debugging macros \n

*/

#include "main.h"

#include "version.h"

#include "debugging/debugging.h"

#include "iundo.h"
#include "iregistry.h"

#include <gtk/gtkmain.h>

#include "cmdlib.h"
#include "os/file.h"
#include "os/path.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

#include "gtkutil/messagebox.h"
#include "gtkutil/image.h"
#include "console.h"
#include "texwindow.h"
#include "map.h"
#include "mainframe.h"
#include "preferences.h"
#include "environment.h"
#include "referencecache.h"
#include "stacktrace.h"
#include "server.h"
#include "ui/mru/MRU.h"

#include <iostream>

void show_splash();
void hide_splash();

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
    write_stack_trace(outputStream);
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

void create_global_pid()
{
  /*!
  the global prefs loading / game selection dialog might fail for any reason we don't know about
  we need to catch when it happens, to cleanup the stateful prefs which might be killing it
  and to turn on console logging for lookup of the problem
  this is the first part of the two step .pid system
  http://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=297
  */
  StringOutputStream g_pidFile(256); ///< the global .pid file (only for global part of the startup)

  g_pidFile << SettingsPath_get() << "radiant.pid";

  FILE *pid;
  pid = fopen (g_pidFile.c_str(), "r");
  if (pid != 0)
  {
    fclose (pid);

    if (remove (g_pidFile.c_str()) == -1)
    {
      StringOutputStream msg(256);
      msg << "WARNING: Could not delete " << g_pidFile.c_str();
      gtk_MessageBox (0, msg.c_str(), "Radiant", eMB_OK, eMB_ICONERROR );
    }

    // in debug, never prompt to clean registry, turn console logging auto after a failed start
#if !defined(_DEBUG)
    StringOutputStream msg(256);
    msg << "Radiant failed to start properly the last time it was run.\n"
           "The failure may be related to current global preferences.\n"
           "Do you want to reset global preferences to defaults?";

    if (gtk_MessageBox (0, msg.c_str(), "Radiant - Startup Failure", eMB_YESNO, eMB_ICONQUESTION) == eIDYES)
    {
      g_GamesDialog.Reset();
    }

    msg.clear();
    msg << "Logging console output to " << SettingsPath_get() << "radiant.log\nRefer to the log if Radiant fails to start again.";

    gtk_MessageBox (0, msg.c_str(), "Radiant - Console Log", eMB_OK);
#endif

    // set without saving, the class is not in a coherent state yet
    // just do the value change and call to start logging, CGamesDialog will pickup when relevant
    g_GamesDialog.m_bForceLogConsole = true;
    Sys_LogFile(true);
  }

  // create a primary .pid for global init run
  pid = fopen (g_pidFile.c_str(), "w");
  if (pid)
    fclose (pid);
}

void remove_global_pid()
{
  StringOutputStream g_pidFile(256);
  g_pidFile << SettingsPath_get() << "radiant.pid";

  // close the primary
  if (remove (g_pidFile.c_str()) == -1)
  {
    StringOutputStream msg(256);
    msg << "WARNING: Could not delete " << g_pidFile.c_str();
    gtk_MessageBox (0, msg.c_str(), "Radiant", eMB_OK, eMB_ICONERROR );
  }
}

/*!
now the secondary game dependant .pid file
http://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=297
*/
void create_local_pid()
{
  StringOutputStream g_pidGameFile(256); ///< the game-specific .pid file
  g_pidGameFile << SettingsPath_get() << g_pGameDescription->mGameFile.c_str() << "/radiant-game.pid";

  FILE *pid = fopen (g_pidGameFile.c_str(), "r");
  if (pid != 0)
  {
    fclose (pid);
    if (remove (g_pidGameFile.c_str()) == -1)
    {
      StringOutputStream msg;
      msg << "WARNING: Could not delete " << g_pidGameFile.c_str();
      gtk_MessageBox (0, msg.c_str(), "Radiant", eMB_OK, eMB_ICONERROR );
    }

    // in debug, never prompt to clean registry, turn console logging auto after a failed start
#if !defined(_DEBUG)
    StringOutputStream msg;
    msg << "Radiant failed to start properly the last time it was run.\n"
           "The failure may be caused by current preferences.\n"
           "Do you want to reset all preferences to defaults?";

    if (gtk_MessageBox (0, msg.c_str(), "Radiant - Startup Failure", eMB_YESNO, eMB_ICONQUESTION) == eIDYES)
    {
      Preferences_Reset();
    }

    msg.clear();
    msg << "Logging console output to " << SettingsPath_get() << "radiant.log\nRefer to the log if Radiant fails to start again.";

    gtk_MessageBox (0, msg.c_str(), "Radiant - Console Log", eMB_OK);
#endif

    // force console logging on! (will go in prefs too)
    g_GamesDialog.m_bForceLogConsole = true;
    Sys_LogFile(true);
  }
  else
  {
    // create one, will remove right after entering message loop
    pid = fopen (g_pidGameFile.c_str(), "w");
    if (pid)
      fclose (pid);
  }
}


/*!
now the secondary game dependant .pid file
http://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=297
*/
void remove_local_pid()
{
  StringOutputStream g_pidGameFile(256);
  g_pidGameFile << SettingsPath_get() << g_pGameDescription->mGameFile.c_str() << "/radiant-game.pid";
  remove(g_pidGameFile.c_str());
}

int main (int argc, char* argv[])
{
  crt_init();

  streams_init();

  gtk_disable_setlocale();
  gtk_init(&argc, &argv);

  GlobalDebugMessageHandler::instance().setHandler(GlobalPopupDebugMessageHandler::instance());

	// Retrieve the application path and such
	Environment::Instance().init(argc, argv);

	// Load the Radiant modules from the modules/ and plugins/ dir.
	ModuleLoader::loadModules(Environment::Instance().getAppPath());

	// Initialise and instantiate the registry
	GlobalModuleServer::instance().set(GlobalModuleServer_get());
	GlobalRegistryModuleRef registryRef;

  show_splash();

  create_global_pid();

  GlobalPreferences_Init();

  g_GamesDialog.Init();

  remove_global_pid();

  g_Preferences.Init(); // must occur before create_local_pid() to allow preferences to be reset

  create_local_pid();

  // in a very particular post-.pid startup
  // we may have the console turned on and want to keep it that way
  // so we use a latching system
  if (g_GamesDialog.m_bForceLogConsole)
  {
    Sys_LogFile(true);
    g_Console_enableLogging = true;
    g_GamesDialog.m_bForceLogConsole = false;
  }


  Radiant_Initialise();

  g_pParentWnd = 0;
  g_pParentWnd = new MainFrame();

  hide_splash();

	if (GlobalMRU().loadLastMap() && GlobalMRU().getLastMapName() != "") {
		Map_LoadFile(GlobalMRU().getLastMapName().c_str());
	}
	else {
		Map_New();
	}

  remove_local_pid();

  gtk_main();

  // avoid saving prefs when the app is minimized
  if (g_pParentWnd->IsSleeping())
  {
    globalOutputStream() << "Shutdown while sleeping, not saving prefs\n";
    g_preferences_globals.disable_ini = true;
  }

  Map_Free();

  delete g_pParentWnd;

  Radiant_Shutdown();

  // close the log file if any
  Sys_LogFile(false);

  return EXIT_SUCCESS;
}

