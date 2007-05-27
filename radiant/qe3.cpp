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

#include <map>

#include "stream/textfilestream.h"
#include "stream/stringstream.h"
#include "string/string.h"
#include "scenelib.h"

#include "gtkutil/messagebox.h"
#include "error.h"
#include "map.h"
#include "mainframe.h"
#include "convert.h"

int g_numbrushes = 0;
int g_numentities = 0;

void QE_UpdateStatusBar() {
	std::string text = "Brushes: " + intToStr(g_numbrushes);
	text += " Entities: " + intToStr(g_numentities);
	g_pParentWnd->SetStatusText(g_pParentWnd->m_brushcount_status, text);
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

