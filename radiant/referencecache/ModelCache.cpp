#include "ModelCache.h"

#include "ifilesystem.h"
#include "imodel.h"
#include "ifiletypes.h"

#include <iostream>
#include "stream/textstream.h"

#include "modulesystem/StaticModule.h"

namespace model {

ModelCache::ModelCache() :
	_enabled(true)
{}

scene::INodePtr ModelCache::find(const std::string& path, const std::string& name) {
	if (!_enabled) {
		return scene::INodePtr();
	}
	
	ModelNodeMap::iterator found = _modelNodeMap.find(ModelKey(path, name));
	
	if (found != _modelNodeMap.end()) {
		return found->second;
	}
	
	return scene::INodePtr();
}
	
void ModelCache::insert(const std::string& path, const std::string& name, const scene::INodePtr& modelNode) {
	if (!_enabled) {
		return; // do nothing if not enabled
	}
	
	// Insert into map and save the iterator
	_modelNodeMap.insert(ModelNodeMap::value_type(ModelKey(path, name), modelNode));
}

void ModelCache::erase(const std::string& path, const std::string& name) {
	if (!_enabled) {
		return;
	}
	
	ModelNodeMap::iterator found = _modelNodeMap.find(ModelKey(path, name));
	
	if (found != _modelNodeMap.end()) {
		_modelNodeMap.erase(found);
	}
}

void ModelCache::clear() {
	// greebo: Disable the modelcache. During map::clear(), the nodes
	// get cleared, which might trigger a loopback to insert().
	_enabled = false;
	
	_modelNodeMap.clear();
	
	// Allow usage of the modelnodemap again.
	_enabled = true;
}

ModelCache& ModelCache::Instance() {
	static ModelCache _instance;
	return _instance;
}

} // namespace model
