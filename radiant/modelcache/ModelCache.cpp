#include "ModelCache.h"

#include "ifilesystem.h"
#include "imodel.h"
#include "ifiletypes.h"

#include "stream/textstream.h"

#include "modulesystem/StaticModule.h"

namespace model {

scene::INodePtr ModelCache::find(const std::string& path) {
	ModelNodeMap::iterator found = _modelNodeMap.find(path);
	
	if (found != _modelNodeMap.end()) {
		return found->second;
	}
	
	return scene::INodePtr();
}
	
void ModelCache::insert(const std::string& path, const scene::INodePtr& modelNode) {
	_modelNodeMap.insert(ModelNodeMap::value_type(path, modelNode));
}

void ModelCache::erase(const std::string& path) {
	ModelNodeMap::iterator found = _modelNodeMap.find(path);
		
	if (found != _modelNodeMap.end()) {
		_modelNodeMap.erase(found);
	}
}

void ModelCache::clear() {
	_modelNodeMap.clear();
}

const std::string& ModelCache::getName() const {
	static std::string _name(MODULE_MODELCACHE);
	return _name;
}

const StringSet& ModelCache::getDependencies() const {
	static StringSet _dependencies;
	
	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_MODELLOADER + "ASE");
		_dependencies.insert(MODULE_MODELLOADER + "MD5MESH");
		_dependencies.insert(MODULE_MODELLOADER + "LWO");
	}
	
	return _dependencies;
}

void ModelCache::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "ModelCache::initialiseModule called.\n";
	
	//g_nullModel = NewNullModel();
}

// Define the ModelCache registerable module
module::StaticModule<ModelCache> modelCacheModule;

} // namespace model
