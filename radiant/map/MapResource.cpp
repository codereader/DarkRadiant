#include "MapResource.h"

#include "ifiletypes.h"
#include "ifilesystem.h"
#include "map/Map.h"
#include "map/RootNode.h"
#include "mapfile.h"
#include "referencecache/NullModelLoader.h"
#include "debugging/debugging.h"
#include "referencecache.h"
#include "os/path.h"
#include "os/file.h"
#include "map/algorithm/Traverse.h"
#include "stream/stringstream.h"

namespace map {

namespace {
	// name may be absolute or relative
	inline std::string rootPath(const std::string& name) {
		return GlobalFileSystem().findRoot(
			path_is_absolute(name.c_str()) ? name : GlobalFileSystem().findFile(name)
		);
	}
}

// Constructor
MapResource::MapResource(const std::string& name) :
	_mapRoot(SingletonNullModel()),
	_originalName(name),
	_type(name.substr(name.rfind(".") + 1)), 
	_modified(0),
	_realised(false)
{
	// Initialise the paths, this is all needed for realisation
    _path = rootPath(_originalName);
	_name = os::getRelativePath(_originalName, _path);
}
	
MapResource::~MapResource() {
    if (realised()) {
		unrealise();
	}
}

bool MapResource::load() {
	ASSERT_MESSAGE(realised(), "resource not realised");
	if (_mapRoot == SingletonNullModel()) {
		// Map not loaded yet, acquire map root node from loader
		_mapRoot = loadMapNode();
		
		connectMap();
		mapSave();
	}

	return _mapRoot != SingletonNullModel();
}
  
/**
 * Save this resource (only for map resources).
 * 
 * @returns
 * true if the resource was saved, false otherwise.
 */
bool MapResource::save() {
	std::string moduleName = GlobalFiletypes().findModuleName("map", _type);
								
	if (!moduleName.empty()) {
		MapFormatPtr format = boost::dynamic_pointer_cast<MapFormat>(
			module::GlobalModuleRegistry().getModule(moduleName)
		);
		
		if (format == NULL) {
			globalErrorStream() << "Could not locate map loader module.\n";
			return false;
		}
		
		// Save a backup of the existing file (rename it to .bak)
		saveBackup();
		
		std::string fullpath = _path + _name;
		
		bool success = false;
		
		if (path_is_absolute(fullpath.c_str())) {
			// Save the actual file
			success = MapResource_saveFile(*format, _mapRoot, map::traverse, fullpath.c_str());
		}
		else {
			globalErrorStream() << "Map path is not absolute: " << makeQuoted(fullpath.c_str()) << "\n";
			success = false;
		}
		
		if (success) {
  			mapSave();
  			return true;
		}
	}
	
	return false;
}

bool MapResource::saveBackup() {
	std::string fullpath = _path + _name;
	
	if (path_is_absolute(fullpath.c_str())) {
		// Save a backup if possible. This is done by renaming the original,
		// which won't work if the existing map is currently open by Doom 3
		// in the background.
		if (!file_exists(fullpath.c_str())) {
			return false;
		}
		
		if (file_writeable(fullpath.c_str())) {
			std::string pathWithoutExtension = fullpath.substr(0, fullpath.rfind('.'));
			std::string backup = pathWithoutExtension + ".bak";
			
			return (!file_exists(backup.c_str()) || file_remove(backup.c_str())) // remove backup
				&& file_move(fullpath.c_str(), backup.c_str()); // rename current to backup
		}
		else {
			globalErrorStream() << "map path is not writeable: " << makeQuoted(fullpath.c_str()) << "\n";
			return false;
		}
	}
	return false;
}

void MapResource::flush() {
	// greebo: Nothing to do, no cache is used for MapResources
}

scene::INodePtr MapResource::getNode() {
	return _mapRoot;
}

void MapResource::setNode(scene::INodePtr node) {
	_mapRoot = node;
	connectMap();
}
	
void MapResource::addObserver(Observer& observer) {
	if (realised()) {
		observer.onResourceRealise();
	}
	_observers.insert(&observer);
}
	
void MapResource::removeObserver(Observer& observer) {
	if (realised()) {
		observer.onResourceUnrealise();
	}
	_observers.erase(&observer);
}
		
bool MapResource::realised() {
	return _realised;
}
  
// Realise this MapResource
void MapResource::realise() {
	if (_realised) {
		return; // nothing to do
	}
	
	_realised = true;

	// Realise the observers
	for (ResourceObserverList::iterator i = _observers.begin();
		 i != _observers.end(); i++)
	{
		(*i)->onResourceRealise();
	}
}
	
void MapResource::unrealise() {
	if (!_realised) {
		return; // nothing to do
	}
	
	_realised = false;
	
	// Realise the observers
	for (ResourceObserverList::iterator i = _observers.begin(); 
		 i != _observers.end(); i++)
	{
		(*i)->onResourceUnrealise();
	}

	//globalOutputStream() << "MapResource::unrealise: " << _path.c_str() << _name.c_str() << "\n";
	_mapRoot = SingletonNullModel();
}

void MapResource::onMapChanged() {
	GlobalMap().setModified(!GlobalReferenceCache().referencesSaved());
}

void MapResource::connectMap() {
    MapFilePtr map = Node_getMapFile(_mapRoot);
    if (map != NULL) {
    	// Reroute the changed callback to the onMapChanged() call.
    	map->setChangedCallback(MapChangedCaller(*this));
    }
    else {
    	map->setChangedCallback(Callback());
    }
}

std::time_t MapResource::modified() const {
	std::string fullpath = _path + _name;
	return file_modified(fullpath.c_str());
}

void MapResource::mapSave() {
	_modified = modified();
	MapFilePtr map = Node_getMapFile(_mapRoot);
	if (map != NULL) {
		map->save();
	}
}

bool MapResource::isModified() const {
	// had or has an absolute path // AND disk timestamp changed
	return (!_path.empty() && _modified != modified()) 
			|| !path_equal(rootPath(_originalName).c_str(), _path.c_str()); // OR absolute vfs-root changed
}

void MapResource::refresh() {
    if (isModified()) {
		flush();
		unrealise();
		realise();
	}
}

MapFormatPtr MapResource::getMapFormat() {
	// Get a loader module name for this type, if possible. If none is 
	// found, try again with the "map" type, since we might be loading a 
	// map with a different extension
    std::string moduleName = GlobalFiletypes().findModuleName("map", _type);
    
	// If empty, try again with "map" type
	if (moduleName.empty()) {
		moduleName = GlobalFiletypes().findModuleName("map", "map"); 
	}

	// If we have a module, use it to load the map if possible, otherwise 
	// return an error
    if (!moduleName.empty()) {
		MapFormatPtr format = boost::dynamic_pointer_cast<MapFormat>(
			module::GlobalModuleRegistry().getModule(moduleName)
		);

		if (format != NULL) {
			// valid MapFormat, return
			return format;
		} 
		else {
			globalErrorStream() << "ERROR: Map type incorrectly registered: \""
				<< moduleName.c_str() << "\"\n";
			return MapFormatPtr();
		}
	} 
    else {
    	globalErrorStream() << "Map loader module not found.\n";
		if (!_type.empty()) {
			globalErrorStream() << "Type is not supported: \""
				<< _name.c_str() << "\"\n";
		}
		return MapFormatPtr();
	}
}

scene::INodePtr MapResource::loadMapNode() {
	// greebo: Check if we have valid settings
	if (_path.empty() || (_name.empty() && _type.empty())) {
		return SingletonNullModel();
	}
	
	// Get the mapformat
	MapFormatPtr format = getMapFormat();
	
	if (format == NULL) {
		return SingletonNullModel(); 
		// error message already printed in getMapFormat();
	}
	
	// At this point, we have a valid mapformat
	// Ccreate a new map root node
	scene::INodePtr root(NewMapRoot(_name));

  	std::string fullpath = _path + _name;

	if (path_is_absolute(fullpath.c_str())) {
		MapResource_loadFile(*format, root, fullpath);
	}
	else {
		globalErrorStream() << "map path is not fully qualified: " << makeQuoted(fullpath.c_str()) << "\n";
	}

	return root;
}

} // namespace map
