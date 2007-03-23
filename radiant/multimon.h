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

#if !defined(INCLUDED_MULTIMON_H)
#define INCLUDED_MULTIMON_H

#include <string>
#include "gtkutil/WindowPosition.h"

/** greebo: This adjusts the window coordinates within the given 
 * 			WindowPosition helper class so that the window
 * 			is located on the primary screen. You will
 * 			have to call the tracker's applyPosition() method afterwards. 
 */
void positionWindowOnPrimaryScreen(gtkutil::WindowPosition& position);

	namespace {
		const std::string RKEY_MULTIMON_START_PRIMARY = "user/ui/multiMonitor/startOnPrimaryMonitor";
		const std::string RKEY_MULTIMON_DISABLE_SYS_MENU = "user/ui/multiMonitor/disableSysMenuOnPopups";
	}

#if defined(WIN32)
void MultiMon_Construct();
void MultiMon_Destroy();
#else
inline void MultiMon_Construct()
{
}
inline void MultiMon_Destroy()
{
}
#endif

#endif
