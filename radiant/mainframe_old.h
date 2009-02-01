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

#if !defined(INCLUDED_MAINFRAME_H)
#define INCLUDED_MAINFRAME_H

#include "gtkutil/window.h"
#include "gtkutil/idledraw.h"
#include "gtkutil/widget.h"
#include "gtkutil/PanedPosition.h"
#include "gtkutil/WindowPosition.h"
#include "string/string.h"

#include "iradiant.h"

#include "ui/mainframe/MainFrame.h"

void ScreenUpdates_Disable(const char* message, const char* title = "");
void ScreenUpdates_Enable();
bool ScreenUpdates_Enabled();
void ScreenUpdates_process();

class ScopeDisableScreenUpdates
{
public:
  ScopeDisableScreenUpdates(const char* message, const char* title = "")
  {
    ScreenUpdates_Disable(message, title);
  }
  ~ScopeDisableScreenUpdates()
  {
    ScreenUpdates_Enable();
  }
};

void Radiant_Initialise();

void SaveMapAs();

void GlobalCamera_UpdateWindow();
void UpdateAllWindows();

void updateTextureBrowser();
void ClipperChangeNotify();

void DefaultMode();

void MainFrame_Construct();

#endif
