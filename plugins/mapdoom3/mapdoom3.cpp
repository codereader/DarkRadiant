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

#include "ibrush.h"
#include "ipatch.h"
#include "ifiletypes.h"
#include "ieclass.h"
#include "iregistry.h"
#include "iradiant.h"
#include "ishaders.h"

#include "imodule.h"
#include "scenelib.h"
#include "parser/DefTokeniser.h"

#include "parse.h"
#include "write.h"

#include <boost/lexical_cast.hpp>

namespace {
	const std::string RKEY_PRECISION = "game/mapFormat/floatPrecision";
	const int MAPVERSION = 2;
}

class MapDoom3API : 
	public MapFormat,
	public PrimitiveParser
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("Doom3MapLoader");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_RADIANT);
			_dependencies.insert(MODULE_FILETYPES);
			_dependencies.insert(MODULE_ECLASSMANAGER);
			_dependencies.insert(MODULE_SCENEGRAPH);
			_dependencies.insert(MODULE_BRUSHCREATOR);
			_dependencies.insert(MODULE_PATCH + DEF2);
			_dependencies.insert(MODULE_PATCH + DEF3);
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_SHADERSYSTEM);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "MapDoom3API::initialiseModule called.\n";
		
		GlobalFiletypes().addType(
    		"map", getName(), FileTypePattern("Doom 3 map", "*.map"));
    	GlobalFiletypes().addType(
    		"map", getName(), FileTypePattern("Doom 3 region", "*.reg"));
    	GlobalFiletypes().addType(
    		"map", getName(), FileTypePattern("Doom 3 prefab", "*.pfb"));
    	
    	// Add the filepatterns for the prefab (different order)
    	GlobalFiletypes().addType(
    		"prefab", getName(), FileTypePattern("Doom 3 prefab", "*.pfb"));
    	GlobalFiletypes().addType(
    		"prefab", getName(), FileTypePattern("Doom 3 map", "*.map"));
    	GlobalFiletypes().addType(
    		"prefab", getName(), FileTypePattern("Doom 3 region", "*.reg"));
	}
	
	/**
	 * Parse a primitive from the given token stream.
	 */
	scene::INodePtr parsePrimitive(parser::DefTokeniser& tokeniser) const {
	    std::string primitive = tokeniser.nextToken();
	    
	    if (primitive == "patchDef3") {
	        return GlobalPatchCreator(DEF3).createPatch();
	    }
	    else if (primitive == "patchDef2") {
	        return GlobalPatchCreator(DEF2).createPatch();
	    }
	    else if(primitive == "brushDef3") {
	    	return GlobalBrushCreator().createBrush();
	    }
	    else {
	        return scene::INodePtr();
	    }
  }
  
    /**
     * Read tokens from a map stream and create entities accordingly.
     */
    void readGraph(scene::INodePtr root, TextInputStream& inputStream) const {
        // Construct a tokeniser
        std::istream is(&inputStream);
        parser::BasicDefTokeniser<std::istream> tok(is);
        
        // Parse the map version
        float version = 0;
        try {
            tok.assertNextToken("Version");
            version = boost::lexical_cast<float>(tok.nextToken());
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
        if (version != MAPVERSION) {
            globalErrorStream() 
                << "Incorrect map version: required " << MAPVERSION 
                << ", found " << version << "\n";
            return;
        }
        
        // Now start parsing the map
        Map_Read(root, tok, *this);
    }

	// Write scene graph to an ostream
	void writeGraph(scene::INodePtr root, GraphTraversalFunc traverse, std::ostream& os) const {
		int precision = GlobalRegistry().getInt(RKEY_PRECISION);  
		os.precision(precision);
	    os << "Version " << MAPVERSION << std::endl;
		Map_Write(root, traverse, os);
	}
};
typedef boost::shared_ptr<MapDoom3API> MapDoom3APIPtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(MapDoom3APIPtr(new MapDoom3API));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getOutputStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
