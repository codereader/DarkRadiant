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

#include "modulesystem/singletonmodule.h"

#include "ShaderTemplate.h"
#include "shaders.h"

class ShadersDependencies :
  public GlobalFileSystemModuleRef,
  public GlobalTexturesModuleRef,
  public GlobalRadiantModuleRef,
  public GlobalRegistryModuleRef
{};

class ShadersDoom3API
{
  ShaderSystem* m_shadersdoom3;
public:
  typedef ShaderSystem Type;
  STRING_CONSTANT(Name, "doom3");

  ShadersDoom3API(ShadersDependencies& dependencies)
  {
    Shaders_Construct();
    m_shadersdoom3 = &GetShaderSystem();
  }
  ~ShadersDoom3API()
  {
    Shaders_Destroy();
  }
  ShaderSystem* getTable()
  {
    return m_shadersdoom3;
  }
};

typedef SingletonModule<ShadersDoom3API, ShadersDependencies, DependenciesAPIConstructor<ShadersDoom3API, ShadersDependencies> > ShadersDoom3Module;

ShadersDoom3Module g_ShadersDoom3Module;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
  initialiseModule(server);

  g_ShadersDoom3Module.selfRegister();
}
