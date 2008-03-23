#include "Doom3MapFormat.h"

#include "ifiletypes.h"
#include "ieclass.h"
#include "ibrush.h"
#include "ipatch.h"
#include "iregistry.h"

#include "NodeExporter.h"
#include "parser/DefTokeniser.h"
#include "stream/textstream.h"
#include "MapExportInfo.h"

#include <boost/lexical_cast.hpp>

namespace map {

	namespace {
		const std::string RKEY_PRECISION = "game/mapFormat/floatPrecision";
		const int MAPVERSION = 2;
	}

// RegisterableModule implementation
const std::string& Doom3MapFormat::getName() const {
	static std::string _name("Doom3MapLoader");
	return _name;
}

const StringSet& Doom3MapFormat::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_ECLASSMANAGER);
		_dependencies.insert(MODULE_BRUSHCREATOR);
		_dependencies.insert(MODULE_PATCH + DEF2);
		_dependencies.insert(MODULE_PATCH + DEF3);
		_dependencies.insert(MODULE_XMLREGISTRY);
	}

	return _dependencies;
}

void Doom3MapFormat::initialiseModule(const ApplicationContext& ctx) {
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
scene::INodePtr Doom3MapFormat::parsePrimitive(parser::DefTokeniser& tokeniser) const {
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
void Doom3MapFormat::readGraph(scene::INodePtr root, TextInputStream& inputStream) const {
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
void Doom3MapFormat::writeGraph(const map::MapExportInfo& exportInfo) const {
	int precision = GlobalRegistry().getInt(RKEY_PRECISION);
	exportInfo.mapStream.precision(precision);

	// Write the version tag first
    exportInfo.mapStream << "Version " << MAPVERSION << std::endl;

	// Instantiate a NodeExporter class and call the traverse function
	NodeExporter exporter(exportInfo.mapStream);
	exportInfo.traverse(exportInfo.root, exporter);
}

} // namespace map
