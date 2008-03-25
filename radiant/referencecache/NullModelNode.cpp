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
		_nullModelNode->setSelf(_nullModelNode);
	}

	return _nullModelNode;
}

void NullModelNode::instantiate(const scene::Path& path) {
	Node::instantiate(path);
}

void NullModelNode::uninstantiate(const scene::Path& path) {
	Node::uninstantiate(path);
}
  
const IModel& NullModelNode::getIModel() const {
	return *_nullModel;
}

void NullModelNode::testSelect(Selector& selector, SelectionTest& test) {
	_nullModel->testSelect(selector, test, localToWorld());
}

void NullModelNode::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	_nullModel->renderSolid(renderer, volume, localToWorld());
}

void NullModelNode::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	_nullModel->renderWireframe(renderer, volume, localToWorld());
}

const AABB& NullModelNode::localAABB() const {
	return _nullModel->localAABB();
}

VolumeIntersectionValue NullModelNode::intersectVolume(
	const VolumeTest& test, const Matrix4& localToWorld) const
{
	return _nullModel->intersectVolume(test, localToWorld);
}

} // namespace model
