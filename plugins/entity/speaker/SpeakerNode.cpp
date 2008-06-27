#include "SpeakerNode.h"

#include "math/frustum.h"

namespace entity {

SpeakerNode::SpeakerNode(IEntityClassPtr eclass) :
	EntityNode(eclass),
	TransformModifier(Speaker::TransformChangedCaller(m_contained), ApplyTransformCaller(*this)),
	TargetableNode(_entity, *this),
	m_contained(eclass, 
		*this, 
		Node::TransformChangedCaller(*this), 
		Node::BoundsChangedCaller(*this),
		EvaluateTransformCaller(*this))
{
	TargetableNode::construct();
}

SpeakerNode::SpeakerNode(const SpeakerNode& other) :
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
	TransformModifier(Speaker::TransformChangedCaller(m_contained), ApplyTransformCaller(*this)),
	TargetableNode(_entity, *this),
	m_contained(other.m_contained, 
		*this, 
		Node::TransformChangedCaller(*this), 
		Node::BoundsChangedCaller(*this),
		EvaluateTransformCaller(*this))
{
	TargetableNode::construct();
}

SpeakerNode::~SpeakerNode() {
	TargetableNode::destruct();
}

// Snappable implementation
void SpeakerNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// Bounded implementation
const AABB& SpeakerNode::localAABB() const {
	return m_contained.localAABB();
}

// TransformNode implementation
const Matrix4& SpeakerNode::localToParent() const {
	return m_contained.getTransformNode().localToParent();
}

// Cullable implementation
VolumeIntersectionValue SpeakerNode::intersectVolume(
    const VolumeTest& test, const Matrix4& localToWorld) const
{
	return m_contained.intersectVolume(test, localToWorld);
}

// EntityNode implementation
Entity& SpeakerNode::getEntity() {
	return m_contained.getEntity();
}

void SpeakerNode::refreshModel() {
	// Nothing to do
}

// Namespaced implementation
/*void SpeakerNode::setNamespace(INamespace& space) {
	m_contained.getNamespaced().setNamespace(space);
}*/

void SpeakerNode::testSelect(Selector& selector, SelectionTest& test) {
	m_contained.testSelect(selector, test, localToWorld());
}

scene::INodePtr SpeakerNode::clone() const {
	scene::INodePtr clone(new SpeakerNode(*this));
	clone->setSelf(clone);
	return clone;
}

void SpeakerNode::instantiate(const scene::Path& path) {
	m_contained.instanceAttach(path);
	Node::instantiate(path);
}

void SpeakerNode::uninstantiate(const scene::Path& path) {
	m_contained.instanceDetach(path);
	Node::uninstantiate(path);
}

// Nameable implementation
std::string SpeakerNode::name() const {
	return m_contained.getNameable().name();
}

void SpeakerNode::attach(const NameCallback& callback) {
	m_contained.getNameable().attach(callback);
}

void SpeakerNode::detach(const NameCallback& callback) {
	m_contained.getNameable().detach(callback);
}

void SpeakerNode::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderSolid(renderer, volume, localToWorld());
}
void SpeakerNode::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderWireframe(renderer, volume, localToWorld());
}

void SpeakerNode::evaluateTransform() {
	if(getType() == TRANSFORM_PRIMITIVE) {
		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());
	}
}

void SpeakerNode::applyTransform() {
	m_contained.revertTransform();
	evaluateTransform();
	m_contained.freezeTransform();
}

} // namespace entity
