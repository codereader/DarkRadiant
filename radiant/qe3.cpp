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

/*
The following source code is licensed by Id Software and subject to the terms of 
its LIMITED USE SOFTWARE LICENSE AGREEMENT, a copy of which is included with 
GtkRadiant. If you did not receive a LIMITED USE SOFTWARE LICENSE AGREEMENT, 
please contact Id Software immediately at info@idsoftware.com.
*/

//
// Linux stuff
//
// Leonardo Zide (leo@lokigames.com)
//

#include "qe3.h"

#include "debugging/debugging.h"

#include "ifilesystem.h"

#include <map>

#include <gtk/gtktearoffmenuitem.h>

#include "stream/textfilestream.h"
#include "cmdlib.h"
#include "stream/stringstream.h"
#include "os/path.h"
#include "scenelib.h"

#include "gtkutil/messagebox.h"
#include "error.h"
#include "map.h"
#include "points.h"
#include "camera/CamWnd.h"
#include "mainframe.h"
#include "preferences.h"
#include "convert.h"

QEGlobals_t  g_qeglobals;


#if defined(WIN32)
#define PATH_MAX 260
#endif


void QE_InitVFS()
{
  // VFS initialization -----------------------
  // we will call GlobalFileSystem().initDirectory, giving the directories to look in (for files in pk3's and for standalone files)
  // we need to call in order, the mod ones first, then the base ones .. they will be searched in this order
  // *nix systems have a dual filesystem in ~/.q3a, which is searched first .. so we need to add that too
	std::string gamename = gamename_get();
	std::string basegame = basegame_get();
#if defined(POSIX)
	std::string userRoot = g_qeglobals.m_userEnginePath.c_str();
#endif
	std::string globalRoot = EnginePath_get();

	// if we have a mod dir (gamename is not equal)
	if (gamename != basegame) {
#if defined(POSIX)
		{
			// ~/.<gameprefix>/<fs_game>
			std::string userGamePath = userRoot + gamename + "/";
			GlobalFileSystem().initDirectory(userGamePath);
	    }
#endif

		// <fs_basepath>/<fs_game>
		{
			std::string globalGamePath = globalRoot + gamename + "/";
			GlobalFileSystem().initDirectory(globalGamePath);
	    }
	}

#if defined(POSIX)
	// ~/.<gameprefix>/<fs_main>
	{
		std::string userBasePath = userRoot + basegame + "/";
		GlobalFileSystem().initDirectory(userBasePath);
	}
#endif

	// <fs_basepath>/<fs_main>
	{
		std::string globalBasePath = globalRoot + basegame + "/";
		GlobalFileSystem().initDirectory(globalBasePath);
	}
}

int g_numbrushes = 0;
int g_numentities = 0;

void QE_UpdateStatusBar()
{
  char buffer[128];
  sprintf(buffer, "Brushes: %d Entities: %d", g_numbrushes, g_numentities);
  g_pParentWnd->SetStatusText(g_pParentWnd->m_brushcount_status, buffer);
}

SimpleCounter g_brushCount;

void QE_brushCountChanged()
{
  g_numbrushes = int(g_brushCount.get());
  QE_UpdateStatusBar();
}

SimpleCounter g_entityCount;

void QE_entityCountChanged()
{
  g_numentities = int(g_entityCount.get());
  QE_UpdateStatusBar();
}

bool ConfirmModified(const char* title)
{
  if (!Map_Modified(g_map))
    return true;

  EMessageBoxReturn result = gtk_MessageBox(GTK_WIDGET(MainFrame_getWindow()), "The current map has changed since it was last saved.\nDo you want to save the current map before continuing?", title, eMB_YESNOCANCEL, eMB_ICONQUESTION);
  if(result == eIDCANCEL)
  {
    return false;
  }
  if(result == eIDYES)
  {
    if(Map_Unnamed(g_map))
    {
      return Map_SaveAs();
    }
    else
    {
      Map_Save();
    }
  }
  return true;
}


const char* const EXECUTABLE_TYPE = 
#if defined(__linux__) || defined (__FreeBSD__)
"x86"
#elif defined(__APPLE__)
"ppc"
#elif defined(WIN32)
"exe"
#else
#error "unknown platform"
#endif
;

// =============================================================================
// Sys_ functions

void Sys_SetTitle(const char *text, bool modified)
{
  StringOutputStream title;
  title << ConvertLocaleToUTF8(text);

  if(modified)
  {
    title << " *";
  }

  gtk_window_set_title(MainFrame_getWindow(), title.c_str());
}

bool g_bWaitCursor = false;

void Sys_BeginWait (void)
{
  ScreenUpdates_Disable("Processing...", "Please Wait");
  GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor(GTK_WIDGET(MainFrame_getWindow())->window, cursor);
  gdk_cursor_unref (cursor);
  g_bWaitCursor = true;
}

void Sys_EndWait (void)
{
  ScreenUpdates_Enable();
  gdk_window_set_cursor(GTK_WIDGET(MainFrame_getWindow())->window, 0);
  g_bWaitCursor = false;
}

void Sys_Beep (void)
{
  gdk_beep();
}

