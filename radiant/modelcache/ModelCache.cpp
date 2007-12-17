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

ModelCache& ModelCache::Instance() {
	static ModelCache _instance;
	return _instance;
}

} // namespace model
