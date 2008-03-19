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

namespace {
	class ModelRefreshWalker :
		public scene::Graph::Walker
	{
	public:
		virtual bool pre(const scene::Path& path, const scene::INodePtr& node) const {
			EntityNodePtr entity = boost::dynamic_pointer_cast<EntityNode>(node);

			if (entity != NULL) {
				entity->refreshModel();
				return false;
			}

			return true;
		}
	};
}

RadiantReferenceCache::RadiantReferenceCache() : 
	_realised(false)
{}

void RadiantReferenceCache::clear() {
	GlobalModelCache().clear();
	_mapReferences.clear();
}

// Branch for capturing mapfile resources
ReferenceCache::ResourcePtr RadiantReferenceCache::captureMap(const std::string& path) {
	// First lookup the reference in the map. If it is found, we need to
	// lock the weak_ptr to get a shared_ptr, which may fail. If we cannot
	// get a shared_ptr (because the object as already been deleted) or the
	// item is not found at all, we create a new MapResource and add it
	// into the map before returning.
	MapReferences::iterator i = _mapReferences.find(path);
	if (i != _mapReferences.end()) {
		// Found. Try to lock the pointer. If it is valid, return it.
		map::MapResourcePtr candidate = i->second.lock();
		if (candidate) {
			return candidate;
		}
	}
	
	// Either we did not find the resource, or the pointer was not valid.
	// In this case we create a new MapResource, add it to the map and
	// return it.
	map::MapResourcePtr newResource(new map::MapResource(path));
	
	// Realise the new resource if the ReferenceCache itself is realised
	if (realised()) {
		newResource->realise();
	}
	
	// Insert the weak pointer reference into the map
	_mapReferences[path] = map::MapResourceWeakPtr(newResource);
	
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

		for (MapReferences::iterator i = _mapReferences.begin();
	  	     i != _mapReferences.end();
	  	     ++i)
		{
			map::MapResourcePtr resource = i->second.lock();
      		if (resource) {
        		resource->realise();
      		}
    	}
	}
}

void RadiantReferenceCache::unrealise() {
	if (_realised) {
		_realised = false;

		for (MapReferences::iterator i = _mapReferences.begin();
	  	     i != _mapReferences.end();
	  	     ++i)
		{
			map::MapResourcePtr resource = i->second.lock();
      		if (resource) {
        		resource->unrealise();
      		}
    	}

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
  
void RadiantReferenceCache::refreshReferences() {
	ScopeDisableScreenUpdates disableScreenUpdates("Refreshing models");
	
	// Clear the model cache
	GlobalModelCache().clear();

	// Update all model nodes
	GlobalSceneGraph().traverse(ModelRefreshWalker());
		
	// greebo: Reload the modelselector too
	ui::ModelSelector::refresh();
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
	
	GlobalEventManager().addCommand(
		"RefreshReferences", 
		MemberCaller<RadiantReferenceCache, &RadiantReferenceCache::refreshReferences>(*this)
	);
	
	GlobalFileSystem().addObserver(*this);
	realise();
}

void RadiantReferenceCache::shutdownModule() {
	unrealise();
	GlobalFileSystem().removeObserver(*this);
}

void RadiantReferenceCache::saveReferences() {
	for (MapReferences::iterator i = _mapReferences.begin(); 
		 i != _mapReferences.end(); 
		 ++i)
	{
    	map::MapResourcePtr res = i->second.lock();
    	if (res != NULL) {
    		res->save();
    	}
	}
	
	// Map is modified as soon as unsaved references exist
	GlobalMap().setModified(!referencesSaved());
}

bool RadiantReferenceCache::referencesSaved() {
	for (MapReferences::iterator i = _mapReferences.begin(); 
		 i != _mapReferences.end(); ++i)
	{
		scene::INodePtr node;
		
	    map::MapResourcePtr res = i->second.lock();
	    if (res != NULL) {
	    	node = res->getNode();
	    }
	    	
	    if (node != NULL) {
	    	MapFilePtr map = Node_getMapFile(node);
	    	if (map != NULL && !map->saved()) {
	    		return false;
	    	}
	    }
	}

	return true;
}
	
// Define the ReferenceCache registerable module
module::StaticModule<RadiantReferenceCache> referenceCacheModule;
