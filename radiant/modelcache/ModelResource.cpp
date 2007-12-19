#include "ModelResource.h"

#include "ifiletypes.h"
#include "ifilesystem.h"
#include "NullModelLoader.h"
#include "ModelCache.h"
#include "debugging/debugging.h"
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
}

// Constructor
ModelResource::ModelResource(const std::string& name) :
	m_model(SingletonNullModel()),
	m_originalName(name),
	_type(name.substr(name.rfind(".") + 1)), 
	m_modified(0),
	_realised(false)
{}
	
ModelResource::~ModelResource() {
    unrealise(); // unrealise - does nothing if not realised
}

void ModelResource::setModel(scene::INodePtr model) {
	m_model = model;
}

void ModelResource::clearModel() {
	m_model = SingletonNullModel();
}

bool ModelResource::load() {
	ASSERT_MESSAGE(realised(), "resource not realised");
	
	if (m_model == SingletonNullModel()) {
		// Try to lookup the model from the cache
		scene::INodePtr cached = ModelCache::Instance().find(m_path, m_name);
		
		if (cached != NULL) {
			// found a cached model, take this one
			setModel(cached);
		}
		else {
			// Model was not found yet
			scene::INodePtr loaded = loadModelNode();
			
			// Insert the newly loaded model into the cache
			ModelCache::Instance().insert(m_path, m_name, loaded);
			
			setModel(loaded);
		}
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

void ModelResource::refresh() {
    flush();
	unrealise();
	realise();
}

ModelLoaderPtr ModelResource::getModelLoaderForType(const std::string& type) {
	// Get the module name from the Filetype registry
	std::string moduleName = GlobalFiletypes().findModuleName("model", type);
	  
	if (!moduleName.empty()) {
		ModelLoaderPtr modelLoader = boost::static_pointer_cast<ModelLoader>(
			module::GlobalModuleRegistry().getModule(moduleName)
		);

		if (modelLoader != NULL) {
			return modelLoader;
		}
		else {
			globalErrorStream()	<< "ERROR: Model type incorrectly registered: \""
								<< moduleName.c_str() << "\"\n";
			return NullModelLoader::InstancePtr();
		}
	}
	return ModelLoaderPtr();
}

scene::INodePtr ModelResource::loadModelNode() {
	// Get the model loader for this resource type
	ModelLoaderPtr loader = getModelLoaderForType(_type);
	
	// greebo: Check if we have a NULL model loader and an empty path 
	// (name is something like "func_static_637" then)
	if (loader == NULL && m_path.empty()) {
		return SingletonNullModel();
	}

	// Model types should have a loader, so use this to load. Map types do not
	// have a loader		  
	if (loader != NULL) {
		// Construct a NullModel as return value
		scene::INodePtr model(SingletonNullModel());

		ArchiveFilePtr file = GlobalFileSystem().openFile(m_name);

		if (file != NULL) {
			globalOutputStream() << "Loaded Model: \""<< m_name.c_str() << "\"\n";
			model = loader->loadModel(*file);
		}
		else {
			globalErrorStream() << "Model load failed: \""<< m_name.c_str() << "\"\n";
		}

		model->setIsRoot(true);

		return model;
	}
	
	return SingletonNullModel();
}

} // namespace model
