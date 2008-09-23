#include "RadiantReferenceCache.h"

#include "imodel.h"
#include "mapfile.h"
#include "modelskin.h"
#include "ifiletypes.h"
#include "ieventmanager.h"
#include "imap.h"
#include "os/path.h"
#include "ModelCache.h"
#include "debugging/debugging.h"
#include "modulesystem/StaticModule.h"
#include "mainframe.h"
#include "ui/modelselector/ModelSelector.h"
#include "map/Map.h"

#include <boost/algorithm/string/predicate.hpp>

RadiantReferenceCache::RadiantReferenceCache() : 
	_realised(false)
{}

// Branch for capturing mapfile resources
ReferenceCache::ResourcePtr RadiantReferenceCache::captureMap(const std::string& path) {
	// Create a new MapResource and return it.
	map::MapResourcePtr newResource(new map::MapResource(path));
	
	// Realise the new resource if the ReferenceCache itself is realised
	if (realised()) {
		newResource->realise();
	}
	
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
	
bool RadiantReferenceCache::realised() const {
	return _realised;
}
	
void RadiantReferenceCache::realise() {
	ASSERT_MESSAGE(!_realised, "RadiantReferenceCache::realise: already realised");
	
	if (!_realised) {
		_realised = true;
	}
}

void RadiantReferenceCache::unrealise() {
	if (_realised) {
		_realised = false;
		GlobalModelCache().clear();
	}
}

// Gets called on VFS initialise
void RadiantReferenceCache::onFileSystemInitialise() {
	realise();
}
  	
// Gets called on VFS shutdown
void RadiantReferenceCache::onFileSystemShutdown() {
	unrealise();
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
		_dependencies.insert(MODULE_EVENTMANAGER);
	}
	
	return _dependencies;
}

void RadiantReferenceCache::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "ReferenceCache::initialiseModule called.\n";
	
	GlobalFileSystem().addObserver(*this);
	realise();
}

void RadiantReferenceCache::shutdownModule() {
	unrealise();
	GlobalFileSystem().removeObserver(*this);
}

// Define the ReferenceCache registerable module
module::StaticModule<RadiantReferenceCache> referenceCacheModule;
