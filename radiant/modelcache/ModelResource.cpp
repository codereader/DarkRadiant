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
	m_model(SingletonNullModel()),
	m_originalName(name),
	_type(name.substr(name.rfind(".") + 1)), 
	m_loader(NULL),
	m_modified(0),
	_realised(false)
{
	// Get the model loader for this resource type
	m_loader = getModelLoaderForType(_type);
}
	
ModelResource::~ModelResource() {
    unrealise(); // unrealise - does nothing if not realised
}

void ModelResource::setModel(scene::INodePtr model) {
	m_model = model;
}

void ModelResource::clearModel() {
	m_model = SingletonNullModel();
}

void ModelResource::loadCached() {
	scene::INodePtr cached = ModelCache::Instance().find(m_path, m_name);
	  
	if (cached != NULL) {
		// found a cached model
		setModel(cached);
		return;
	}
	  
	// Model was not found yet
	scene::INodePtr loaded = loadModelNode();
	ModelCache::Instance().insert(m_path, m_name, loaded);
	setModel(loaded);
}

void ModelResource::loadModel() {
	loadCached();
}

bool ModelResource::load() {
	ASSERT_MESSAGE(realised(), "resource not realised");
	if (m_model == SingletonNullModel()) {
		loadModel();
	}

	return m_model != SingletonNullModel();
}
  
void ModelResource::flush() {
	if (realised()) {
		ModelCache::Instance().erase(m_path, m_name);
	}
}

scene::INodePtr ModelResource::getNode() {
	return m_model;
}

void ModelResource::setNode(scene::INodePtr node) {
	// Erase the mapping in the modelcache
	ModelCache::Instance().erase(m_path, m_name);
	
	// Re-insert the model with the new node
	ModelCache::Instance().insert(m_path, m_name, node);
	
	setModel(node);
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
	return _realised;
}
  
// Realise this ModelResource
void ModelResource::realise() {
	if (_realised) {
		return; // nothing to do
	}
	
	_realised = true;
	
	// Initialise the paths, this is all needed for realisation
    m_path = rootPath(m_originalName.c_str());
	m_name = os::getRelativePath(m_originalName, m_path);

	// Realise the observers
	for (ResourceObserverList::iterator i = _observers.begin();
		 i != _observers.end(); i++)
	{
		(*i)->onResourceRealise();
	}
}
	
void ModelResource::unrealise() {
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

	clearModel();
}

std::time_t ModelResource::modified() const {
	std::string fullpath = m_path + m_name;
	return file_modified(fullpath.c_str());
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
		return SingletonNullModel();
	}

	// Model types should have a loader, so use this to load. Map types do not
	// have a loader		  
	if (m_loader != NULL) {
		return loadModelResource();
	}
	else if (m_name.empty() && _type.empty()) {
		// Loader is NULL (map) and no valid name and type, return NULLmodel
		return SingletonNullModel();
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
	        return SingletonNullModel();
	      }
	    }
	    else
	    {
	      if (!_type.empty())
	      {
	        globalErrorStream() << "Model type not supported: \"" << m_name.c_str() << "\"\n";
	      }
	      return SingletonNullModel();
	    }
	}
}

scene::INodePtr ModelResource::loadModelResource() {
	scene::INodePtr model(SingletonNullModel());

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
