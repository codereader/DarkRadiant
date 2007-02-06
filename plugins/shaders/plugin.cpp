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

#include "plugin.h"

#include "ishaders.h"
#include "ifilesystem.h"
#include "itextures.h"
#include "iregistry.h"
#include "qerplugin.h"
#include "preferencesystem.h"

#include "modulesystem/singletonmodule.h"

#include "ShaderTemplate.h"
#include "Doom3ShaderSystem.h"

class ShadersDependencies :
	public GlobalFileSystemModuleRef,
	public GlobalTexturesModuleRef,
	public GlobalRadiantModuleRef,
	public GlobalRegistryModuleRef,
	public GlobalPreferenceSystemModuleRef 
{};

class ShadersDoom3API
{
	// The pointer to the shadersystem instance
	shaders::Doom3ShaderSystem* _shaderSystem;

public:
	typedef ShaderSystem Type;
	STRING_CONSTANT(Name, "doom3");

	ShadersDoom3API(ShadersDependencies& dependencies) {
		_shaderSystem = &GetShaderSystem();
		_shaderSystem->construct();
	}

	~ShadersDoom3API() {
		_shaderSystem->destroy();
	}

	ShaderSystem* getTable() {
		return dynamic_cast<ShaderSystem*>(_shaderSystem);
	}
};

typedef SingletonModule<ShadersDoom3API, ShadersDependencies, DependenciesAPIConstructor<ShadersDoom3API, ShadersDependencies> > ShadersDoom3Module;

ShadersDoom3Module g_ShadersDoom3Module;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server) {
	initialiseModule(server);

	g_ShadersDoom3Module.selfRegister();
}
