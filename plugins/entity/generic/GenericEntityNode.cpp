#include "GenericEntityNode.h"

#include "math/frustum.h"

namespace entity {

GenericEntityNode::GenericEntityNode(IEntityClassPtr eclass) :
	EntityNode(eclass),
	TransformModifier(GenericEntity::TransformChangedCaller(m_contained), ApplyTransformCaller(*this)),
	TargetableNode(_entity, *this),
	m_contained(eclass, 
		*this, 
		Node::TransformChangedCaller(*this), 
		EvaluateTransformCaller(*this))
{
	TargetableNode::construct();
}

GenericEntityNode::GenericEntityNode(const GenericEntityNode& other) :
	EntityNode(other),
	SelectableNode(other),
	scene::Cloneable(other),
	Nameable(other),
	Snappable(other),
	TransformNode(other),
	SelectionTestable(other),
	Renderable(other),
	Cullable(other),
	Bounded(other),
	TransformModifier(GenericEntity::TransformChangedCaller(m_contained), ApplyTransformCaller(*this)),
	TargetableNode(_entity, *this),
	m_contained(other.m_contained, 
		*this, 
		Node::TransformChangedCaller(*this), 
		EvaluateTransformCaller(*this))
{
	TargetableNode::construct();
}

GenericEntityNode::~GenericEntityNode() {
	TargetableNode::destruct();
}

// Snappable implementation
void GenericEntityNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// TransformNode implementation
const Matrix4& GenericEntityNode::localToParent() const {
	return m_contained.getTransformNode().localToParent();
}

// EntityNode implementation
Entity& GenericEntityNode::getEntity() {
	return m_contained.getEntity();
}

void GenericEntityNode::refreshModel() {
	// nothing to do
}

// Bounded implementation
const AABB& GenericEntityNode::localAABB() const {
	return m_contained.localAABB();
}

// Cullable implementation
VolumeIntersectionValue GenericEntityNode::intersectVolume(
    const VolumeTest& test, const Matrix4& localToWorld) const
{
	return m_contained.intersectVolume(test, localToWorld);
}

// Namespaced implementation
/*void GenericEntityNode::setNamespace(INamespace& space) {
	m_contained.getNamespaced().setNamespace(space);
}*/

void GenericEntityNode::testSelect(Selector& selector, SelectionTest& test) {
	m_contained.testSelect(selector, test, localToWorld());
}

scene::INodePtr GenericEntityNode::clone() const {
	scene::INodePtr clone(new GenericEntityNode(*this));
	clone->setSelf(clone);
	return clone;
}

void GenericEntityNode::instantiate(const scene::Path& path) {
	m_contained.instanceAttach(path);
	Node::instantiate(path);
}

void GenericEntityNode::uninstantiate(const scene::Path& path) {
	m_contained.instanceDetach(path);
	Node::uninstantiate(path);
}

// Nameable implementation
std::string GenericEntityNode::name() const {
	return m_contained.getNameable().name();
}

void GenericEntityNode::attach(const NameCallback& callback) {
	m_contained.getNameable().attach(callback);
}

void GenericEntityNode::detach(const NameCallback& callback) {
	m_contained.getNameable().detach(callback);
}

void GenericEntityNode::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderSolid(renderer, volume, localToWorld());
}

void GenericEntityNode::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderWireframe(renderer, volume, localToWorld());
}

void GenericEntityNode::evaluateTransform() {
	if(getType() == TRANSFORM_PRIMITIVE) {
		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());
	}
}

void GenericEntityNode::applyTransform() {
	m_contained.revertTransform();
	evaluateTransform();
	m_contained.freezeTransform();
}

} // namespace entity
