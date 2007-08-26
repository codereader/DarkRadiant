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

#include "brush/TexDef.h"
#include "ibrush.h"
#include "ipatch.h"
#include "ifiletypes.h"
#include "ieclass.h"
#include "iregistry.h"
#include "iradiant.h"
#include "ishaders.h"

#include "scenelib.h"
#include "generic/constant.h"
#include "parser/DefTokeniser.h"

#include "modulesystem/singletonmodule.h"

#include "parse.h"
#include "write.h"

#include <boost/lexical_cast.hpp>

namespace {
	const std::string RKEY_PRECISION = "game/mapFormat/floatPrecision";
}

class MapDoom3Dependencies :
  public GlobalRadiantModuleRef,
  public GlobalFiletypesModuleRef,
  public GlobalEntityClassManagerModuleRef,
  public GlobalSceneGraphModuleRef,
  public GlobalBrushModuleRef,
  public GlobalRegistryModuleRef,
  public GlobalShadersModuleRef
{
  PatchModuleRef m_patchDef2Doom3Module;
  PatchModuleRef m_patchDoom3Module;
public:
  MapDoom3Dependencies() :
    GlobalEntityClassManagerModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("entityclass")),
    GlobalBrushModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("brushtypes")),
    GlobalShadersModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("shaders")),
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

class MapDoom3API 
: public MapFormat, 
  public PrimitiveParser
{
  	MapDoom3Dependencies& m_dependencies;
public:

	typedef MapFormat Type;
	STRING_CONSTANT(Name, "mapdoom3");
	INTEGER_CONSTANT(MapVersion, 2);

	/**
	 * Constructor. Register the map filetypes in the GlobalFiletypes module.
	 */
	MapDoom3API(MapDoom3Dependencies& dependencies) 
	: m_dependencies(dependencies)
	{
    	GlobalFiletypes().addType(
    		"map", "mapdoom3", FileTypePattern("Doom 3 map", "*.map"));
    	GlobalFiletypes().addType(
    		"map", "mapdoom3", FileTypePattern("Doom 3 region", "*.reg"));
    	GlobalFiletypes().addType(
    		"map", "mapdoom3", FileTypePattern("Doom 3 prefab", "*.pfb"));
    	
    	// Add the filepatterns for the prefab (different order)
    	GlobalFiletypes().addType(
    		"prefab", "mapdoom3", FileTypePattern("Doom 3 prefab", "*.pfb"));
    	GlobalFiletypes().addType(
    		"prefab", "mapdoom3", FileTypePattern("Doom 3 map", "*.map"));
    	GlobalFiletypes().addType(
    		"prefab", "mapdoom3", FileTypePattern("Doom 3 region", "*.reg"));
	}
	
	MapFormat* getTable() {
		return this;
	}

	/**
	 * Parse a primitive from the given token stream.
	 */
	scene::INodePtr parsePrimitive(parser::DefTokeniser& tokeniser) const {
	    std::string primitive = tokeniser.nextToken();
	    if (primitive == "patchDef3")
	        return m_dependencies.getPatchDoom3().createPatch();
	    else if(primitive == "patchDef2")
	        return m_dependencies.getPatchDef2Doom3().createPatch();
	    else if(primitive == "brushDef3")
	        return m_dependencies.getBrushDoom3().createBrush();
	    else
	        return scene::INodePtr();
  }
  
    /**
     * Read tokens from a map stream and create entities accordingly.
     */
    void readGraph(scene::INodePtr root, 
                   TextInputStream& inputStream, 
                   EntityCreator& entityTable) const
    {
        // Construct a tokeniser
        std::istream is(&inputStream);
        parser::BasicDefTokeniser<std::istream> tok(is);
        
        // Parse the map version
        int version = 0;
        try {
            tok.assertNextToken("Version");
            version = boost::lexical_cast<int>(tok.nextToken());
        }
        catch (parser::ParseException e) {
            globalErrorStream() 
                << "[mapdoom3] Unable to parse map version: " 
                << e.what() << "\n";
            return;
        }
        catch (boost::bad_lexical_cast e) {
            globalErrorStream() 
                << "[mapdoom3] Unable to parse map version: " 
                << e.what() << "\n";
            return;
        }

        // Check we have the correct version for this module
        if (version != MapVersion()) {
            globalErrorStream() 
                << "Incorrect map version: required " << MapVersion() 
                << ", found " << version << "\n";
            return;
        }
        
        // Now start parsing the map
        Map_Read(root, tok, entityTable, *this);
    }

	// Write scene graph to an ostream
	void writeGraph(scene::INodePtr root, GraphTraversalFunc traverse, std::ostream& os) const {
		int precision = GlobalRegistry().getInt(RKEY_PRECISION);  
		os.precision(precision);
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
