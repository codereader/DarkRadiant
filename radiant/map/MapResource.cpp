#include "MapResource.h"

#include "ifiletypes.h"
#include "ifilesystem.h"
#include "map/Map.h"
#include "mapfile.h"
#include "modelcache/NullModelLoader.h"
#include "debugging/debugging.h"
#include "referencecache.h"
#include "os/path.h"
#include "os/file.h"
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
	m_model(SingletonNullModel()),
	m_originalName(name),
	_type(name.substr(name.rfind(".") + 1)), 
	m_modified(0),
	_realised(false)
{
	// Initialise the paths, this is all needed for realisation
    m_path = rootPath(m_originalName);
	m_name = os::getRelativePath(m_originalName, m_path);
}
	
MapResource::~MapResource() {
    if (realised()) {
		unrealise();
	}
}

void MapResource::setModel(scene::INodePtr model) {
	m_model = model;
}

void MapResource::clearModel() {
	m_model = SingletonNullModel();
}

bool MapResource::load() {
	ASSERT_MESSAGE(realised(), "resource not realised");
	if (m_model == SingletonNullModel()) {
		// Map not loaded yet, acquire map root node from loader
		scene::INodePtr loaded = loadMapNode();
		setModel(loaded);
		
		connectMap();
		mapSave();
	}

	return m_model != SingletonNullModel();
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
		
		if (format != NULL && MapResource_save(*format, m_model, m_path, m_name)) {
  			mapSave();
  			return true;
		}
	}
	
	return false;
}
	
void MapResource::flush() {
	// greebo: Nothing to do, no cache is used for MapResources
}

scene::INodePtr MapResource::getNode() {
	return m_model;
}

void MapResource::setNode(scene::INodePtr node) {
	setModel(node);
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

	//globalOutputStream() << "MapResource::unrealise: " << m_path.c_str() << m_name.c_str() << "\n";
	clearModel();
}

void MapResource::onMapChanged() {
	GlobalMap().setModified(!References_Saved());
}

void MapResource::connectMap() {
    MapFilePtr map = Node_getMapFile(m_model);
    if (map != NULL) {
    	// Reroute the changed callback to the onMapChanged() call.
    	map->setChangedCallback(MapChangedCaller(*this));
    }
    else {
    	map->setChangedCallback(Callback());
    }
}

std::time_t MapResource::modified() const {
	std::string fullpath = m_path + m_name;
	return file_modified(fullpath.c_str());
}

void MapResource::mapSave() {
	m_modified = modified();
	MapFilePtr map = Node_getMapFile(m_model);
	if (map != NULL) {
		map->save();
	}
}

bool MapResource::isModified() const {
	// had or has an absolute path // AND disk timestamp changed
	return (!m_path.empty() && m_modified != modified()) 
			|| !path_equal(rootPath(m_originalName).c_str(), m_path.c_str()); // OR absolute vfs-root changed
}

void MapResource::refresh() {
    if (isModified()) {
		flush();
		unrealise();
		realise();
	}
}

scene::INodePtr MapResource::loadMapNode() {
	// greebo: Check if we have an empty path (is true for m_name=="func_static_637")
	if (m_path.empty()) {
		return SingletonNullModel();
	}

	if (m_name.empty() && _type.empty()) {
		// no valid name and type, return NULLmodel
		return SingletonNullModel();
	}
	
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
			return MapResource_load(*format, m_path, m_name);
		} 
		else {
			globalErrorStream() << "ERROR: Map type incorrectly registered: \""
				<< moduleName.c_str() << "\"\n";
			return SingletonNullModel();
		}
	} 
    else {
    	globalErrorStream() << "Map loader module not found.\n";
		if (!_type.empty()) {
			globalErrorStream() << "Type is not supported: \""
				<< m_name.c_str() << "\"\n";
		}
		return SingletonNullModel();
	}
}

} // namespace map
