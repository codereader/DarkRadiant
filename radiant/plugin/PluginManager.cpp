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

#include "PluginManager.h"

#include <list>
#include "modulesystem.h"
#include "qerplugin.h"
#include "plugin.h"

void CPluginManager::fillPluginSlots(GtkWidget* main_window)
{
	class AddPluginVisitor : public PluginModules::Visitor
	{
		CPluginSlots& m_slots;
		GtkWidget* m_main_window;
	public:
		AddPluginVisitor(CPluginSlots& slots, GtkWidget* main_window)
      	: m_slots(slots), m_main_window(main_window)
    	{
    	}
    	
    	void visit(const char* name, const _QERPluginTable& table) {
      		m_slots.AddPluginSlot(m_main_window, name, table);
    	}
  	} visitor(_pluginSlots, main_window);

	// Obtain the plugin modules and visit them with AddPluginVisitor
  	Radiant_getPluginModules().foreachModule(visitor);
}

void CPluginManager::Dispatch(std::size_t n, const std::string& command) {
	_pluginSlots.Dispatch(n, command);
}

void CPluginManager::Init(GtkWidget* main_window) {
	fillPluginSlots(main_window);
}

void CPluginManager::constructMenu(PluginsVisitor& menu) {
	_pluginSlots.PopulateMenu(menu);
}

void CPluginManager::Shutdown() {
}

// =================================================================================

// The function returning the static instance of the plugin manager
CPluginManager& GetPlugInMgr() {
	static CPluginManager _plugInMgr;
  	return _plugInMgr;
}
