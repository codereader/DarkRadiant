#include "Doom3MapFormat.h"

#include "ifiletypes.h"
#include "ieclass.h"
#include "ibrush.h"
#include "ipatch.h"
#include "iregistry.h"

#include "parser/DefTokeniser.h"
#include "stream/textstream.h"

#include "NodeImporter.h"
#include "NodeExporter.h"
#include "scenelib.h"

#include "Tokens.h"
#include "MapImportInfo.h"
#include "MapExportInfo.h"
#include "InfoFile.h"
#include "AssignLayerMappingWalker.h"

#include <boost/lexical_cast.hpp>

namespace map {

	namespace {
		const std::string RKEY_PRECISION = "game/mapFormat/floatPrecision";
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
		_dependencies.insert(MODULE_LAYERSYSTEM);
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

void Doom3MapFormat::readGraph(const MapImportInfo& importInfo) const {
	assert(importInfo.root != NULL);

	// Read the infofile
	InfoFile infoFile(importInfo.infoStream);

	try {
		// Start parsing, this will throw if any errors occur
		infoFile.parse();
	}
	catch (parser::ParseException e) {
        globalErrorStream() << "[mapdoom3] Unable to parse info file: " << e.what() << "\n";
    }

	// Construct a MapImporter that will do the map parsing
	NodeImporter importer(importInfo, infoFile, *this);
	importer.parse();

	// Create the layers according to the data found in the map information file
	const InfoFile::LayerNameMap& layers = infoFile.getLayerNames();

	for (InfoFile::LayerNameMap::const_iterator i = layers.begin(); 
		 i != layers.end(); i++)
	{
		// Create the named layer with the saved ID
		GlobalLayerSystem().createLayer(i->second, i->first);
	}
	
	// Now that the graph is in place, assign the layers
	AssignLayerMappingWalker walker(infoFile);
	importInfo.root->traverse(walker);
}

void Doom3MapFormat::writeGraph(const MapExportInfo& exportInfo) const {
	int precision = GlobalRegistry().getInt(RKEY_PRECISION);
	exportInfo.mapStream.precision(precision);

	// Write the version tag first
    exportInfo.mapStream << VERSION << " " << MAPVERSION << std::endl;

	// Instantiate a NodeExporter class and call the traverse function
	NodeExporter exporter(exportInfo.mapStream, exportInfo.infoStream);
	exportInfo.traverse(exportInfo.root, exporter);
}

} // namespace map
