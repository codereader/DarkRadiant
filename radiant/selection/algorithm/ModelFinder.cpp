#include "ModelFinder.h"

#include "ientity.h"
#include "scenelib.h"

namespace selection {
	namespace algorithm {
	
ModelFinder::ModelFinder() :
	_onlyModels(true)
{}

void ModelFinder::visit(const scene::INodePtr& node) const {
	Entity* entity = Node_getEntity(node);

	if (entity != NULL && entity->isModel()) {
		_modelList.push_back(node);
	}
	else {
		_onlyModels = false;
	}
}

ModelFinder::ModelList& ModelFinder::getList() {
	return _modelList;
}

bool ModelFinder::empty() const {
	return _modelList.empty();
}

bool ModelFinder::onlyModels() const {
	return _onlyModels;
}
		
	} // namespace algorithm
} // namespace selection 
