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

#include "debugging/debugging.h"

#include "iscenegraph.h"
#include "irender.h"
#include "iselection.h"
#include "ientity.h"
#include "iundo.h"
#include "igrid.h"
#include "ieclass.h"
#include "igl.h"
#include "ireference.h"
#include "iregistry.h"
#include "ifilter.h"
#include "preferencesystem.h"
#include "iradiant.h"
#include "namespace.h"
#include "modelskin.h"

#include "typesystem.h"

#include "entity.h"

#include "modulesystem/singletonmodule.h"

class EntityDependencies :
  public GlobalRadiantModuleRef,
  public GlobalOpenGLModuleRef,
  public GlobalUndoModuleRef,
  public GlobalSceneGraphModuleRef,
  public GlobalShaderCacheModuleRef,
  public GlobalSelectionModuleRef,
  public GlobalReferenceModuleRef,
  public GlobalFilterModuleRef,
  public GlobalPreferenceSystemModuleRef,
  public GlobalNamespaceModuleRef,
  public GlobalModelSkinCacheModuleRef,
  public GlobalRegistryModuleRef,
  public GlobalGridModuleRef
{
};

class EntityDoom3API : public TypeSystemRef
{
  EntityCreator* m_entitydoom3;
public:
  typedef EntityCreator Type;
  STRING_CONSTANT(Name, "doom3");

  EntityDoom3API()
  {
    Entity_Construct(eGameTypeDoom3);

    m_entitydoom3 = &GetEntityCreator();

    GlobalReferenceCache().setEntityCreator(*m_entitydoom3);
  }
  ~EntityDoom3API()
  {
    Entity_Destroy();
  }
  EntityCreator* getTable()
  {
    return m_entitydoom3;
  }
};

typedef SingletonModule<EntityDoom3API, EntityDependencies> EntityDoom3Module;

EntityDoom3Module g_EntityDoom3Module;


extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
  initialiseModule(server);

  g_EntityDoom3Module.selfRegister();
}
