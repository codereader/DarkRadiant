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

#include "iscenegraph.h"
#include "irender.h"
#include "iselection.h"
#include "iimage.h"
#include "imodel.h"
#include "igl.h"
#include "ifilesystem.h"
#include "iundo.h"
#include "ifiletypes.h"
#include "iscriplib.h"

#include "modulesystem/singletonmodule.h"
#include "typesystem.h"

#include "md3.h"
#include "mdl.h"
#include "md2.h"
#include "mdc.h"
#include "mdlimage.h"
#include "md5.h"


class ModelDependencies :
  public GlobalFileSystemModuleRef,
  public GlobalOpenGLModuleRef,
  public GlobalUndoModuleRef,
  public GlobalSceneGraphModuleRef,
  public GlobalShaderCacheModuleRef,
  public GlobalSelectionModuleRef,
  public GlobalFiletypesModuleRef
{
};

class MD5ModelLoader : public ModelLoader
{
public:
  scene::Node& loadModel(ArchiveFile& file)
  {
    return loadMD5Model(file);
  }
  
	// Not implemented
	model::IModelPtr loadModelFromPath(const std::string& name) {}
};

class ModelMD5Dependencies : public ModelDependencies, public GlobalScripLibModuleRef
{
};

class ModelMD5API : public TypeSystemRef
{
  MD5ModelLoader m_modelmd5;
public:
  typedef ModelLoader Type;
  STRING_CONSTANT(Name, "md5mesh");

  ModelMD5API()
  {
    GlobalFiletypesModule::getTable().addType(Type::Name(), Name(), filetype_t("md5 meshes", "*.md5mesh"));
  }
  ModelLoader* getTable()
  {
    return &m_modelmd5;
  }
};

typedef SingletonModule<ModelMD5API, ModelMD5Dependencies> ModelMD5Module;

ModelMD5Module g_ModelMD5Module;


extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
  initialiseModule(server);

  g_ModelMD5Module.selfRegister();
}
