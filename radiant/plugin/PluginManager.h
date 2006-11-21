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

/* greebo: The plugin system is organised like this:
 * 
 * Each PlugIn is "stored" in a CPluginSlot class. All of these slots are stored
 * in a list owned by the surrounding CPlugInSlots class (note the plural s :).
 * 
 * The class CPlugInSlots is in turn controlled by the CPluginManager class, which
 * passes its calles to the according CPlugInSlots methods. 
 * 
 * If the "world" calls a method, the call is dispatched this way:
 * "World" > CPluginManager > CPlugInSlots > CPluginSlot 
 *  
 */
 
#if !defined(INCLUDED_PLUGINMANAGER_H)
#define INCLUDED_PLUGINMANAGER_H

#include <cstddef>
#include <string>
#include "PluginsVisitor.h"
#include "PluginSlots.h"

// Forward declaration to avoid including the whole GTK headers
typedef struct _GtkWidget GtkWidget;

class CPluginManager {
	
private:
	// The private list of pluginSlots
	CPluginSlots _pluginSlots;
	
public:
	// Initialises the Manager by filling all the PluginSlots
	void Init(GtkWidget* main_window);
	
	// Dispatches the <command> to all the plugins
	void Dispatch(std::size_t n, const std::string& command);
	
	// Populate a menu using the passed pluginvisitor class
	void constructMenu(PluginsVisitor& menu);
		
	void Shutdown();
	
private:
	void fillPluginSlots(GtkWidget* main_window);
};

// This is the gateway function to access the PluginManager in the radiant core
CPluginManager& GetPlugInMgr();

#endif
