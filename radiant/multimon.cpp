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

#include "gtkutil/MultiMonitor.h"
#include "ipreferencesystem.h"
#include "stringio.h"

#include <gdk/gdkdisplay.h>

void Multimon_registerPreferencesPage() {
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Interface/Multi Monitor");
	
	page->appendCheckBox("", "Start on Primary Monitor", RKEY_MULTIMON_START_PRIMARY);
}

void MultiMon_Construct()
{
	globalOutputStream() << "Default screen has " << gtkutil::MultiMonitor::getNumMonitors() << " monitors.\n";

	// detect multiple monitors
	for (int j = 0; j < gtkutil::MultiMonitor::getNumMonitors(); j++) {
		GdkRectangle geom = gtkutil::MultiMonitor::getMonitor(j);

		globalOutputStream() << "Monitor " << j << " geometry: " 
							 << geom.x << ", " << geom.y << ", " 
							 << geom.width << ", " << geom.height << "\n";
	}

	Multimon_registerPreferencesPage();
}

void MultiMon_Destroy()
{
}
