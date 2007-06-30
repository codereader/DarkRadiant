#include "LightNode.h"

// --------- LightNode implementation ------------------------------------

LightNode::LightNode(IEntityClassPtr eclass) :
	m_contained(eclass, 
				*this, 
				InstanceSet::TransformChangedCaller(m_instances), 
				InstanceSet::BoundsChangedCaller(m_instances), 
				InstanceSetEvaluateTransform<LightInstance>::Caller(m_instances))
{}

LightNode::LightNode(const LightNode& other) :
	scene::Node(other),
	scene::Instantiable(other),
	scene::Cloneable(other),
	Nameable(other),
	Snappable(other),
	Editable(other),
	TransformNode(other),
	EntityNode(other),
	Namespaced(other),
	m_contained(other.m_contained, 
				*this, 
				InstanceSet::TransformChangedCaller(m_instances), 
				InstanceSet::BoundsChangedCaller(m_instances), 
				InstanceSetEvaluateTransform<LightInstance>::Caller(m_instances))
{}

const Matrix4& LightNode::getLocalPivot() const {
	return m_contained.getLocalPivot();
}

// Snappable implementation
void LightNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// TransformNode implementation
const Matrix4& LightNode::localToParent() const {
	return m_contained.getTransformNode().localToParent();
}

Entity& LightNode::getEntity() {
	return m_contained.getEntity();
}

void LightNode::setNamespace(INamespace& space) {
	m_contained.getNamespaced().setNamespace(space);
}

scene::INodePtr LightNode::clone() const {
	return scene::INodePtr(new LightNode(*this));
}

scene::Instance* LightNode::create(const scene::Path& path, scene::Instance* parent) {
	return new LightInstance(path, parent, m_contained);
}

void LightNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

void LightNode::insert(const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(path, instance);
}

scene::Instance* LightNode::erase(const scene::Path& path) {
	return m_instances.erase(path);
}

// Nameable implementation
std::string LightNode::name() const {
	return m_contained.getNameable().name();
}

void LightNode::attach(const NameCallback& callback) {
	m_contained.getNameable().attach(callback);
}

void LightNode::detach(const NameCallback& callback) {
	m_contained.getNameable().detach(callback);
}
