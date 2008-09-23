#include "RadiantReferenceCache.h"

#include "ifilesystem.h"
#include "ifiletypes.h"
#include "itextstream.h"
#include "os/path.h"
#include "modulesystem/StaticModule.h"

// Branch for capturing mapfile resources
ReferenceCache::ResourcePtr RadiantReferenceCache::captureMap(const std::string& path) {
	// Create a new MapResource and return it.
	map::MapResourcePtr newResource(new map::MapResource(path));
	
	// Realise the new resource
	newResource->realise();
	
	return newResource;
}

ReferenceCache::ResourcePtr RadiantReferenceCache::capture(const std::string& path) {
	// The path is recognised as map if the FileTypeRegistry has associated  
	// the extension with the "map" moduletype.
	if (!GlobalFiletypes().findModuleName("map", os::getExtension(path)).empty()) {
		return captureMap(path);
	}

	return ReferenceCache::ResourcePtr();
}
	
// RegisterableModule implementation
const std::string& RadiantReferenceCache::getName() const {
	static std::string _name(MODULE_REFERENCECACHE);
	return _name;
}
	
const StringSet& RadiantReferenceCache::getDependencies() const {
	static StringSet _dependencies;
	
	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert("Doom3MapLoader");
	}
	
	return _dependencies;
}

void RadiantReferenceCache::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "ReferenceCache::initialiseModule called.\n";
}

// Define the ReferenceCache registerable module
module::StaticModule<RadiantReferenceCache> referenceCacheModule;
