#include "ModelCache.h"

#include "ifilesystem.h"
#include "imodel.h"
#include "ifiletypes.h"
#include "ieventmanager.h"

#include <iostream>
#include "os/path.h"
#include "os/file.h"
#include "stream/textstream.h"
#include "mainframe.h"

#include "modulesystem/StaticModule.h"
#include "ui/modelselector/ModelSelector.h"
#include "NullModelLoader.h"

namespace model {

namespace {

	class ModelRefreshWalker :
		public scene::Graph::Walker
	{
	public:
		virtual bool pre(const scene::Path& path, const scene::INodePtr& node) const {
			IEntityNodePtr entity = boost::dynamic_pointer_cast<IEntityNode>(node);

			if (entity != NULL) {
				entity->refreshModel();
				return false;
			}

			return true;
		}
	};

} // namespace

ModelCache::ModelCache() :
	_enabled(true)
{}

ModelLoaderPtr ModelCache::getModelLoaderForType(const std::string& type) {
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
								<< moduleName << "\"\n";
		}
	}

	return NullModelLoader::InstancePtr();
}

scene::INodePtr ModelCache::getModelNode(const std::string& modelPath) {
	// Check if we have a reference to a modeldef
	IModelDefPtr modelDef = GlobalEntityClassManager().findModel(modelPath);

	// The actual model path (is usually the same as the incoming modelPath)
	std::string actualModelPath(modelPath);

	if (modelDef != NULL) {
		// We have a valid modelDef, override the model path
		actualModelPath = modelDef->mesh;
	}

	// Get the extension of this model
	std::string type = actualModelPath.substr(actualModelPath.rfind(".") + 1); 

	// Get a suitable model loader
	ModelLoaderPtr modelLoader = getModelLoaderForType(type);

	// Try to construct a model node using the suitable loader
	scene::INodePtr modelNode = modelLoader->loadModel(actualModelPath);

	if (modelNode != NULL) {
		// Model load was successful
		return modelNode;
	}

	// The model load failed, let's return the NullModel
	// This call should never fail, i.e. the returned model is non-NULL
	return NullModelLoader::InstancePtr()->loadModel(actualModelPath);
}

IModelPtr ModelCache::getModel(const std::string& modelPath) {
	// Try to lookup the existing model
	ModelMap::iterator found = _modelMap.find(modelPath);

	if (_enabled && found != _modelMap.end()) {
		// Try to lock the weak pointer
		IModelPtr model = found->second.lock();

		if (model != NULL) {
			// Model is cached and weak pointer could be locked, return
			return model;
		}

		// Weak pointer could not be locked, remove from the map
		_modelMap.erase(found);
	}

	// The model is not cached, the weak pointer could not be locked
	// or the cache is disabled, load afresh
	
	// Get the extension of this model
	std::string type = modelPath.substr(modelPath.rfind(".") + 1); 

	// Find a suitable model loader
	ModelLoaderPtr modelLoader = getModelLoaderForType(type);

	IModelPtr model = modelLoader->loadModelFromPath(modelPath);

	if (model != NULL) {
		// Model successfully loaded, insert a weak reference into the map
		_modelMap.insert(
			ModelMap::value_type(modelPath, IModelWeakPtr(model))
		);
	}
		
	return model;
}

void ModelCache::clear() {
	// greebo: Disable the modelcache. During map::clear(), the nodes
	// get cleared, which might trigger a loopback to insert().
	_enabled = false;
	
	_modelMap.clear();
	
	// Allow usage of the modelnodemap again.
	_enabled = true;
}

void ModelCache::refreshModels() {
	ScopeDisableScreenUpdates disableScreenUpdates("Refreshing models");
	
	// Clear the model cache
	clear();

	// Update all model nodes
	GlobalSceneGraph().traverse(ModelRefreshWalker());
		
	// greebo: Reload the modelselector too
	ui::ModelSelector::refresh();
}

// RegisterableModule implementation
const std::string& ModelCache::getName() const {
	static std::string _name("ModelCache");
	return _name;
}

const StringSet& ModelCache::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_MODELLOADER + "ASE");
		_dependencies.insert(MODULE_MODELLOADER + "LWO");
		_dependencies.insert(MODULE_MODELLOADER + "MD5MESH");
	}

	return _dependencies;
}

void ModelCache::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "ModelCache::initialiseModule called.\n";

	GlobalEventManager().addCommand(
		"RefreshModels", 
		MemberCaller<ModelCache, &ModelCache::refreshModels>(*this)
	);
}

void ModelCache::shutdownModule() {
	clear();
}

// The static module
module::StaticModule<ModelCache> modelCacheModule; 

} // namespace model
