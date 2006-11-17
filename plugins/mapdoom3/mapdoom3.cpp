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

#include "iscriplib.h"
#include "ibrush.h"
#include "ipatch.h"
#include "ifiletypes.h"
#include "ieclass.h"
#include "qerplugin.h"

#include "scenelib.h"
#include "string/string.h"
#include "stringio.h"
#include "generic/constant.h"

#include "modulesystem/singletonmodule.h"

#include "parse.h"
#include "write.h"


class MapDoom3Dependencies :
  public GlobalRadiantModuleRef,
  public GlobalFiletypesModuleRef,
  public GlobalScripLibModuleRef,
  public GlobalEntityClassManagerModuleRef,
  public GlobalSceneGraphModuleRef,
  public GlobalBrushModuleRef
{
  PatchModuleRef m_patchDef2Doom3Module;
  PatchModuleRef m_patchDoom3Module;
public:
  MapDoom3Dependencies() :
    GlobalEntityClassManagerModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("entityclass")),
    GlobalBrushModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("brushtypes")),
    m_patchDef2Doom3Module("def2doom3"),
    m_patchDoom3Module("doom3")
  {
  }
  BrushCreator& getBrushDoom3()
  {
    return GlobalBrushModule::getTable();
  }
  PatchCreator& getPatchDoom3()
  {
    return *m_patchDoom3Module.getTable();
  }
  PatchCreator& getPatchDef2Doom3()
  {
    return *m_patchDef2Doom3Module.getTable();
  }
};

class MapDoom3API : public TypeSystemRef, public MapFormat, public PrimitiveParser
{
  MapDoom3Dependencies& m_dependencies;
public:
  typedef MapFormat Type;
  STRING_CONSTANT(Name, "mapdoom3");
  INTEGER_CONSTANT(MapVersion, 2);

  MapDoom3API(MapDoom3Dependencies& dependencies) : m_dependencies(dependencies)
  {
    GlobalFiletypesModule::getTable().addType(Type::Name(), Name(), filetype_t("doom3 maps", "*.map"));
    GlobalFiletypesModule::getTable().addType(Type::Name(), Name(), filetype_t("doom3 region", "*.reg"));
  }
  MapFormat* getTable()
  {
    return this;
  }

  scene::Node& parsePrimitive(Tokeniser& tokeniser) const
  {
    const char* primitive = tokeniser.getToken();
    if(primitive != 0)
    {
      if(string_equal(primitive, "patchDef3"))
      {
        return m_dependencies.getPatchDoom3().createPatch();
      }
      else if(string_equal(primitive, "patchDef2"))
      {
        return m_dependencies.getPatchDef2Doom3().createPatch();
      }
      else if(string_equal(primitive, "brushDef3"))
      {
        return m_dependencies.getBrushDoom3().createBrush();
      }
    }

    Tokeniser_unexpectedError(tokeniser, primitive, "#doom3-primitive");
    return g_nullNode;
  }
  void readGraph(scene::Node& root, TextInputStream& inputStream, EntityCreator& entityTable) const
  {
    Tokeniser& tokeniser = GlobalScripLibModule::getTable().m_pfnNewSimpleTokeniser(inputStream);
    tokeniser.nextLine();
    if(!Tokeniser_parseToken(tokeniser, "Version"))
    {
      return;
    }
    std::size_t version;
    if(!Tokeniser_getSize(tokeniser, version))
    {
      return;
    }
    if(version != MapVersion())
    {
      globalErrorStream() << "Doom 3 map version " << MapVersion() << " supported, version is " << Unsigned(version) << "\n";
      return;
    }
    tokeniser.nextLine();
    Map_Read(root, tokeniser, entityTable, *this);
    tokeniser.release();
  }

	// Write scene graph to an ostream
	void writeGraph(scene::Node& root, GraphTraversalFunc traverse, std::ostream& os) const {
	    os << "Version " << MapVersion() << std::endl;
		Map_Write(root, traverse, os);
	}
};

typedef SingletonModule<
  MapDoom3API,
  MapDoom3Dependencies,
  DependenciesAPIConstructor<MapDoom3API, MapDoom3Dependencies>
>
MapDoom3Module;

MapDoom3Module g_MapDoom3Module;


extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
  initialiseModule(server);

  g_MapDoom3Module.selfRegister();
}
