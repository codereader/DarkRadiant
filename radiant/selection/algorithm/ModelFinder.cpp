#include "ModelFinder.h"

#include "ientity.h"
#include "scenelib.h"

namespace selection {
	namespace algorithm {
	
ModelFinder::ModelFinder() :
	_onlyModels(true)
{}

void ModelFinder::visit(scene::Instance& instance) const {
	Entity* entity = Node_getEntity(instance.path().top());
	if (entity != NULL && entity->isModel()) {
		_modelList.push_back(instance.path());
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
