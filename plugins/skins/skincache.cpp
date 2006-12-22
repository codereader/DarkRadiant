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

#include "Doom3SkinCache.h"

#include "ifilesystem.h"
#include "modulesystem/singletonmodule.h"

/* Module dependencies and registration */

class Doom3ModelSkinCacheDependencies : 
public GlobalFileSystemModuleRef 
{
};

typedef SingletonModule<skins::Doom3SkinCache, 
						Doom3ModelSkinCacheDependencies> 
Doom3ModelSkinCacheModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
	// Static skins module instance
	static Doom3ModelSkinCacheModule _module;
	
	// Register the module with the module server
	initialiseModule(server);
	_module.selfRegister();
}


