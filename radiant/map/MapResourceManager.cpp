#include "MapResourceManager.h"

#include "ifilesystem.h"
#include "ifiletypes.h"
#include "itextstream.h"
#include "os/path.h"
#include "modulesystem/StaticModule.h"

IMapResourcePtr MapResourceManager::capture(const std::string& path) {
	// The path is recognised as map if the FileTypeRegistry has associated  
	// the extension with the "map" moduletype.
	if (!GlobalFiletypes().findModuleName("map", os::getExtension(path)).empty()) {
		// Create a new MapResource and return it.
		map::MapResourcePtr newResource(new map::MapResource(path));
		
		// Realise the new resource
		newResource->realise();
		
		return newResource;
	}

	return IMapResourcePtr();
}
	
// RegisterableModule implementation
const std::string& MapResourceManager::getName() const {
	static std::string _name(MODULE_MAPRESOURCEMANAGER);
	return _name;
}
	
const StringSet& MapResourceManager::getDependencies() const {
	static StringSet _dependencies;
	
	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert("Doom3MapLoader");
	}
	
	return _dependencies;
}

void MapResourceManager::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "MapResourceManager::initialiseModule called.\n";
}

// Define the MapResourceManager registerable module
module::StaticModule<MapResourceManager> mapResourceManagerModule;
