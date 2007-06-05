/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

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

#include "multimon.h"

#include "iregistry.h"
#include "debugging/debugging.h"

#include "gtkutil/window.h"
#include "gtkutil/widget.h"
#include "ipreferencesystem.h"
#include "stringio.h"

#include <gdk/gdkdisplay.h>

	namespace {
		GdkRectangle primaryMonitor;
	}

void positionWindowOnPrimaryScreen(gtkutil::WindowPosition& position) {
	gtkutil::PositionVector pos = position.getPosition();
	gtkutil::SizeVector size = position.getSize();
	
	if (size[0] >= primaryMonitor.width - 12) {
		size[0] = primaryMonitor.width - 12;
	}
	
	if (size[1] >= primaryMonitor.height - 24) {
		size[1] = primaryMonitor.height - 48;
	}
	
	if (pos[0] <= primaryMonitor.x || 
		pos[0] + size[0] >= (primaryMonitor.x + primaryMonitor.width) - 12) 
	{
		pos[0] = primaryMonitor.x + 6;
	}
	
	if (pos[1] <= primaryMonitor.y || 
		pos[1] + size[1] >= (primaryMonitor.y + primaryMonitor.height) - 48)
	{
		pos[1] = primaryMonitor.y + 24;
	}
	
	position.setPosition(pos[0], pos[1]);
	position.setSize(size[0], size[1]);
}

void Multimon_registerPreferencesPage() {
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Interface/Multi Monitor");
	
	GtkWidget* primary_monitor = page->appendCheckBox("", 
		"Start on Primary Monitor", RKEY_MULTIMON_START_PRIMARY);
	GtkWidget* popup = page->appendCheckBox("", 
		"Disable system menu on popup windows", RKEY_MULTIMON_DISABLE_SYS_MENU);
	
	Widget_connectToggleDependency(popup, primary_monitor);
}

void MultiMon_Construct()
{
  // detect multiple monitors

  GdkScreen* screen = gdk_display_get_default_screen(gdk_display_get_default());
  gint m = gdk_screen_get_n_monitors(screen);
  globalOutputStream() << "default screen has " << m << " monitors\n";
  for(int j = 0; j != m; ++j)
  {
    GdkRectangle geom;
    gdk_screen_get_monitor_geometry(screen, j, &geom);
    globalOutputStream() << "monitor " << j << " geometry: " << geom.x << ", " << geom.y << ", " << geom.width << ", " << geom.height << "\n";
    if(j == 0)
    {
      // I am making the assumption that monitor 0 is always the primary monitor on win32. Tested on WinXP with gtk+-2.4.
      primaryMonitor = geom;
    }
  }

	if (m > 1) {
		GlobalRegistry().set(RKEY_MULTIMON_START_PRIMARY, "1");
	}

	Multimon_registerPreferencesPage();
}
void MultiMon_Destroy()
{
}
