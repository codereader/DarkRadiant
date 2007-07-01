#include "EclassModelNode.h"

#include "EclassModelInstance.h"

namespace entity {

EclassModelNode::EclassModelNode(IEntityClassPtr eclass) :
	m_contained(eclass, 
				*this, // pass <self> as scene::Traversable&
				InstanceSet::TransformChangedCaller(m_instances), 
				InstanceSetEvaluateTransform<EclassModelInstance>::Caller(m_instances))
{
	construct();
}

EclassModelNode::EclassModelNode(const EclassModelNode& other) :
	scene::Node(other),
	scene::Instantiable(other),
	scene::Cloneable(other),
	Nameable(other),
	Snappable(other),
	TransformNode(other),
	TraversableNodeSet(), // don't copy the TraversableNodeSet from the other node
	EntityNode(other),
	Namespaced(other),
	ModelSkin(other),
	m_contained(other.m_contained, 
				*this, // pass <self> as scene::Traversable&
				InstanceSet::TransformChangedCaller(m_instances), 
				InstanceSetEvaluateTransform<EclassModelInstance>::Caller(m_instances))
{
	
	construct();
}

EclassModelNode::~EclassModelNode() {
	destroy();
}

void EclassModelNode::construct() {
	// Attach the InstanceSet as Traversable::Observer to the nodeset
	TraversableNodeSet::attach(&m_instances);
}

void EclassModelNode::destroy() {
	TraversableNodeSet::detach(&m_instances);
}

// Snappable implementation
void EclassModelNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// TransformNode implementation
const Matrix4& EclassModelNode::localToParent() const {
	return m_contained.getTransformNode().localToParent();
}

// EntityNode implementation
Entity& EclassModelNode::getEntity() {
	return m_contained.getEntity();
}

// Namespaced implementation
void EclassModelNode::setNamespace(INamespace& space) {
	m_contained.getNamespaced().setNamespace(space);
}

void EclassModelNode::attach(ModuleObserver& observer) {
	m_contained.getModelSkin().attach(observer);
}

void EclassModelNode::detach(ModuleObserver& observer) {
	m_contained.getModelSkin().detach(observer);
}

std::string EclassModelNode::getRemap(const std::string& name) const {
	return m_contained.getModelSkin().getRemap(name);
}

scene::INodePtr EclassModelNode::clone() const {
	return scene::INodePtr(new EclassModelNode(*this));
}

scene::Instance* EclassModelNode::create(const scene::Path& path, scene::Instance* parent) {
	return new EclassModelInstance(path, parent, m_contained);
}

void EclassModelNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

void EclassModelNode::insert(const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(path, instance);
}

scene::Instance* EclassModelNode::erase(const scene::Path& path) {
	return m_instances.erase(path);
}

// Nameable implementation
std::string EclassModelNode::name() const {
	return m_contained.getNameable().name();
}

void EclassModelNode::attach(const NameCallback& callback) {
	m_contained.getNameable().attach(callback);
}

void EclassModelNode::detach(const NameCallback& callback) {
	m_contained.getNameable().detach(callback);
}

} // namespace entity
