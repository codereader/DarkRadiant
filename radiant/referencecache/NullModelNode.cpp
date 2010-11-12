#include "NullModelNode.h"

#include "math/frustum.h"

namespace model {

NullModelNode::NullModelNode() :
	_nullModel(new NullModel)
{}

NullModelNode::NullModelNode(const NullModelPtr& nullModel) :
	_nullModel(nullModel)
{}

NullModelNodePtr NullModelNode::InstancePtr() {
	static NullModelNodePtr _nullModelNode;

	if (_nullModelNode == NULL) {
		// Not yet instantiated, create a new NullModel
		_nullModelNode = NullModelNodePtr(new NullModelNode);
	}

	return _nullModelNode;
}

const IModel& NullModelNode::getIModel() const {
	return *_nullModel;
}

void NullModelNode::testSelect(Selector& selector, SelectionTest& test) {
	_nullModel->testSelect(selector, test, localToWorld());
}

void NullModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const {
	_nullModel->renderSolid(collector, volume, localToWorld());
}

void NullModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const {
	_nullModel->renderWireframe(collector, volume, localToWorld());
}

const AABB& NullModelNode::localAABB() const {
	return _nullModel->localAABB();
}

} // namespace model
