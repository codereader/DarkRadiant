#include "ModelResource.h"

#include "ifiletypes.h"
#include "ifilesystem.h"
#include "map/Map.h"
#include "mapfile.h"
#include "NullModelLoader.h"
#include "ModelCache.h"
#include "debugging/debugging.h"
#include "referencecache.h"
#include "os/path.h"
#include "os/file.h"
#include "stream/stringstream.h"
extern scene::INodePtr g_nullModel;

namespace model {

namespace {
  // name may be absolute or relative
	inline const char* rootPath(const char* name) {
		return GlobalFileSystem().findRoot(
			path_is_absolute(name) ? name : GlobalFileSystem().findFile(name)
		);
	}
	
	void MapChanged() {
		GlobalMap().setModified(!References_Saved());
	}
}

// Constructor
ModelResource::ModelResource(const std::string& name) :
	m_model(g_nullModel), 
	m_originalName(name),
	_type(name.substr(name.rfind(".") + 1)), 
	m_loader(NULL),
	m_modified(0),
	m_unrealised(1)
{
	// Get the model loader for this resource type
	m_loader = getModelLoaderForType(_type);
}
	
ModelResource::~ModelResource() {
    if (realised()) {
		unrealise();
	}
}

void ModelResource::setModel(scene::INodePtr model) {
	m_model = model;
}

void ModelResource::clearModel() {
	m_model = g_nullModel;
}

void ModelResource::loadCached() {
   	//std::cout << "looking up model: " << m_name << "\n";
	scene::INodePtr cached = ModelCache::Instance().find(m_name);
	  
	if (cached != NULL) {
		// found a cached model
		//std::cout << "Model found, inserting: " << m_name << "\n";
		setModel(cached);
		return;
	}
	  
	//std::cout << "Model NOT found, inserting: " << m_name << "\n";
	// Model was not found yet
	scene::INodePtr loaded = loadModelNode();
	ModelCache::Instance().insert(m_name, loaded);
	setModel(loaded);
}

void ModelResource::loadModel() {
	loadCached();
	connectMap();
	mapSave();
}

bool ModelResource::load() {
	ASSERT_MESSAGE(realised(), "resource not realised");
	if (m_model == g_nullModel) {
		loadModel();
	}

	return m_model != g_nullModel;
}
  
/**
 * Save this resource (only for map resources).
 * 
 * @returns
 * true if the resource was saved, false otherwise.
 */
bool ModelResource::save() {
	std::string moduleName = GlobalFiletypes().findModuleName("map", _type);
								
	if(!moduleName.empty()) {
		const MapFormat* format = boost::static_pointer_cast<MapFormat>(
			module::GlobalModuleRegistry().getModule(moduleName)
		).get();
		
		if (format != NULL && MapResource_save(*format, m_model, m_path, m_name)) {
  			mapSave();
  			return true;
		}
	}
	
	return false;
}
	
void ModelResource::flush() {
	if (realised()) {
		ModelCache::Instance().erase(m_name);
	}
}

scene::INodePtr ModelResource::getNode() {
	return m_model;
}

void ModelResource::setNode(scene::INodePtr node) {
	// Erase the mapping in the modelcache
	ModelCache::Instance().erase(m_name);
	
	// Re-insert the model with the new node
	ModelCache::Instance().insert(m_name, node);
	
	setModel(node);
	connectMap();
}
	
void ModelResource::addObserver(Observer& observer) {
	if (realised()) {
		observer.onResourceRealise();
	}
	_observers.insert(&observer);
}
	
void ModelResource::removeObserver(Observer& observer) {
	if (realised()) {
		observer.onResourceUnrealise();
	}
	_observers.erase(&observer);
}
		
bool ModelResource::realised() {
	return m_unrealised == 0;
}
  
// Realise this ModelResource
void ModelResource::realise() {
    if(--m_unrealised == 0) {

		m_path = rootPath(m_originalName.c_str());
		m_name = path_make_relative(m_originalName.c_str(), m_path.c_str());

		// Realise the observers
		for (ResourceObserverList::iterator i = _observers.begin();
			 i != _observers.end(); i++)
		{
			(*i)->onResourceRealise();
		}
	}
}
	
void ModelResource::unrealise() {
	if (++m_unrealised == 1) {
		// Realise the observers
		for (ResourceObserverList::iterator i = _observers.begin(); 
			 i != _observers.end(); i++)
		{
			(*i)->onResourceUnrealise();
		}

		//globalOutputStream() << "ModelResource::unrealise: " << m_path.c_str() << m_name.c_str() << "\n";
		clearModel();
	}
}

bool ModelResource::isMap() const {
	return Node_getMapFile(m_model) != 0;
}

void ModelResource::connectMap() {
    MapFilePtr map = Node_getMapFile(m_model);
    if(map != NULL)
    {
      map->setChangedCallback(FreeCaller<MapChanged>());
    }
}

std::time_t ModelResource::modified() const {
	StringOutputStream fullpath(256);
	fullpath << m_path.c_str() << m_name.c_str();
	return file_modified(fullpath.c_str());
}

void ModelResource::mapSave() {
	m_modified = modified();
	MapFilePtr map = Node_getMapFile(m_model);
	if (map != NULL) {
		map->save();
	}
}

bool ModelResource::isModified() const {
	return ((!string_empty(m_path.c_str()) // had or has an absolute path
			&& m_modified != modified()) // AND disk timestamp changed
			|| !path_equal(rootPath(m_originalName.c_str()), m_path.c_str())); // OR absolute vfs-root changed
}

void ModelResource::refresh() {
    if (isModified()) {
		flush();
		unrealise();
		realise();
	}
}

ModelLoader* ModelResource::getModelLoaderForType(const std::string& type) {
	// Get the module name from the Filetype registry
	std::string moduleName = GlobalFiletypes().findModuleName("model", type);
	  
	if (!moduleName.empty()) {
		ModelLoader* table = boost::static_pointer_cast<ModelLoader>(
			module::GlobalModuleRegistry().getModule(moduleName)
		).get();

		if (table != NULL) {
			return table;
		}
		else {
			globalErrorStream()	<< "ERROR: Model type incorrectly registered: \""
								<< moduleName.c_str() << "\"\n";
			return &NullModelLoader::Instance();
		}
	}
	return NULL;
}

scene::INodePtr ModelResource::loadModelNode() {
	// greebo: Check if we have a NULL model loader and an empty path ("func_static_637")
	if (m_loader == NULL && m_path.empty()) {
		return g_nullModel;
	}

	// Model types should have a loader, so use this to load. Map types do not
	// have a loader		  
	if (m_loader != NULL) {
		return loadModelResource();
	}
	else if (m_name.empty() && _type.empty()) {
		// Loader is NULL (map) and no valid name and type, return NULLmodel
		return g_nullModel;
	}
	else {
		// Get a loader module name for this type, if possible. If none is 
		// found, try again with the "map" type, since we might be loading a 
		// map with a different extension
	    std::string moduleName = GlobalFiletypes().findModuleName("map", _type);
		// Empty, try again with "map" type
		if (moduleName.empty()) {
			moduleName = GlobalFiletypes().findModuleName("map", "map"); 
		}
	
		// If we have a module, use it to load the map if possible, otherwise 
		// return an error
	    if (!moduleName.empty()) {
	      
			const MapFormat* format = boost::static_pointer_cast<MapFormat>(
				module::GlobalModuleRegistry().getModule(moduleName)
			).get();
										
	      if (format != NULL)
	      {
	        return MapResource_load(*format, m_path, m_name);
	      }
	      else
	      {
	        globalErrorStream() << "ERROR: Map type incorrectly registered: \"" << moduleName.c_str() << "\"\n";
	        return g_nullModel;
	      }
	    }
	    else
	    {
	      if (!_type.empty())
	      {
	        globalErrorStream() << "Model type not supported: \"" << m_name.c_str() << "\"\n";
	      }
	      return g_nullModel;
	    }
	}
}

scene::INodePtr ModelResource::loadModelResource() {
	scene::INodePtr model(g_nullModel);

	ArchiveFilePtr file = GlobalFileSystem().openFile(m_name);

	if (file != NULL) {
		globalOutputStream() << "Loaded Model: \""<< m_name.c_str() << "\"\n";
		model = m_loader->loadModel(*file);
	}
	else {
		globalErrorStream() << "Model load failed: \""<< m_name.c_str() << "\"\n";
	}

	model->setIsRoot(true);

	return model;
}

} // namespace model
