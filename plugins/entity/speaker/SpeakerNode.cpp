#include "SpeakerNode.h"

#include "SpeakerInstance.h"

namespace entity {

SpeakerNode::SpeakerNode(IEntityClassPtr eclass) :
	m_contained(eclass, 
		*this, 
		InstanceSet::TransformChangedCaller(m_instances), 
		InstanceSet::BoundsChangedCaller(m_instances),
		InstanceSetEvaluateTransform<SpeakerInstance>::Caller(m_instances))
{}

SpeakerNode::SpeakerNode(const SpeakerNode& other) :
	scene::Node(other),
	scene::Instantiable(other),
	scene::Cloneable(other),
	Nameable(other),
	Snappable(other),
	TransformNode(other),
	EntityNode(other),
	Namespaced(other),
	m_contained(other.m_contained, 
		*this, 
		InstanceSet::TransformChangedCaller(m_instances), 
		InstanceSet::BoundsChangedCaller(m_instances),
		InstanceSetEvaluateTransform<SpeakerInstance>::Caller(m_instances))
{}

// Snappable implementation
void SpeakerNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// TransformNode implementation
const Matrix4& SpeakerNode::localToParent() const {
	return m_contained.getTransformNode().localToParent();
}

// EntityNode implementation
Entity& SpeakerNode::getEntity() {
	return m_contained.getEntity();
}

// Namespaced implementation
void SpeakerNode::setNamespace(INamespace& space) {
	m_contained.getNamespaced().setNamespace(space);
}

scene::INodePtr SpeakerNode::clone() const {
	return scene::INodePtr(new SpeakerNode(*this));
}

scene::Instance* SpeakerNode::create(const scene::Path& path, scene::Instance* parent) {
	return new SpeakerInstance(path, parent, m_contained);
}

void SpeakerNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

void SpeakerNode::insert(const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(path, instance);
}

scene::Instance* SpeakerNode::erase(const scene::Path& path) {
	return m_instances.erase(path);
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

} // namespace entity
