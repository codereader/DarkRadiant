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
#include "iscriplib.h"
#include "iregistry.h"
#include "qerplugin.h"

#include "string/string.h"
#include "modulesystem/singletonmodule.h"

#include "ShaderTemplate.h"
#include "shaders.h"

class ShadersDependencies :
  public GlobalFileSystemModuleRef,
  public GlobalTexturesModuleRef,
  public GlobalScripLibModuleRef,
  public GlobalRadiantModuleRef,
  public GlobalRegistryModuleRef
{
  ImageModuleRef m_bitmapModule;
public:
  ShadersDependencies() :
    m_bitmapModule("bmp")
  {
  }
  ImageModuleRef& getBitmapModule()
  {
    return m_bitmapModule;
  }
};

class ShadersDoom3API
{
  ShaderSystem* m_shadersdoom3;
public:
  typedef ShaderSystem Type;
  STRING_CONSTANT(Name, "doom3");

  ShadersDoom3API(ShadersDependencies& dependencies)
  {
    g_enableDefaultShaders = false;
    g_shaderLanguage = SHADERLANGUAGE_DOOM3;
    g_bitmapModule = dependencies.getBitmapModule().getTable();
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


class ShadersQuake4API
{
  ShaderSystem* m_shadersquake4;
public:
  typedef ShaderSystem Type;
  STRING_CONSTANT(Name, "quake4");

  ShadersQuake4API(ShadersDependencies& dependencies)
  {
    g_enableDefaultShaders = false;
    g_shaderLanguage = SHADERLANGUAGE_QUAKE4;
    g_bitmapModule = dependencies.getBitmapModule().getTable();
    Shaders_Construct();
    m_shadersquake4 = &GetShaderSystem();
  }
  ~ShadersQuake4API()
  {
    Shaders_Destroy();
  }
  ShaderSystem* getTable()
  {
    return m_shadersquake4;
  }
};

typedef SingletonModule<ShadersQuake4API, ShadersDependencies, DependenciesAPIConstructor<ShadersQuake4API, ShadersDependencies> > ShadersQuake4Module;

ShadersQuake4Module g_ShadersQuake4Module;



extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
  initialiseModule(server);

  g_ShadersDoom3Module.selfRegister();
  g_ShadersQuake4Module.selfRegister();
}
