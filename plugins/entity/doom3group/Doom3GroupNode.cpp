#include "Doom3GroupNode.h"

#include "Doom3GroupInstance.h"

namespace entity {

Doom3GroupNode::Doom3GroupNode(IEntityClassPtr eclass) :
	m_node(this, this, StaticTypeCasts::instance().get()),
	m_contained(
		eclass,
		m_node,
		InstanceSet::TransformChangedCaller(m_instances),
		InstanceSet::BoundsChangedCaller(m_instances),
		InstanceSetEvaluateTransform<Doom3GroupInstance>::Caller(m_instances)
	) 
{
	construct();
}

Doom3GroupNode::Doom3GroupNode(const Doom3GroupNode& other) :
	scene::Node::Symbiot(other),
	scene::Instantiable(other),
	scene::Cloneable(other),
	scene::Traversable::Observer(other),
	m_node(this, this, StaticTypeCasts::instance().get()),
	m_contained(
		other.m_contained,
		m_node,
		InstanceSet::TransformChangedCaller(m_instances),
		InstanceSet::BoundsChangedCaller(m_instances),
		InstanceSetEvaluateTransform<Doom3GroupInstance>::Caller(m_instances)
	) 
{
	construct();
}

void Doom3GroupNode::construct() {
	m_contained.attach(this);
}

void Doom3GroupNode::destroy() {
	m_contained.detach(this);
}

Doom3GroupNode::~Doom3GroupNode() {
	destroy();
}

void Doom3GroupNode::release() {
	delete this;
}

scene::Node& Doom3GroupNode::node() {
	return m_node;
}

scene::Node& Doom3GroupNode::clone() const {
	return (new Doom3GroupNode(*this))->node();
}

void Doom3GroupNode::insert(scene::Node& child) {
	m_instances.insert(child);
}
void Doom3GroupNode::erase(scene::Node& child) {
	m_instances.erase(child);
}

scene::Instance* Doom3GroupNode::create(const scene::Path& path, scene::Instance* parent) {
	return new Doom3GroupInstance(path, parent, m_contained);
}

void Doom3GroupNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

void Doom3GroupNode::insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(observer, path, instance);
}

scene::Instance* Doom3GroupNode::erase(scene::Instantiable::Observer* observer, const scene::Path& path) {
	return m_instances.erase(observer, path);
}

scene::Traversable& Doom3GroupNode::get(NullType<scene::Traversable>) {
	return m_contained.getTraversable();
}

Snappable& Doom3GroupNode::get(NullType<Snappable>) {
	return m_contained;
}

TransformNode& Doom3GroupNode::get(NullType<TransformNode>) {
	return m_contained.getTransformNode();
}

Entity& Doom3GroupNode::get(NullType<Entity>) {
	return m_contained.getEntity();
}

Nameable& Doom3GroupNode::get(NullType<Nameable>) {
	return m_contained.getNameable();
}

Namespaced& Doom3GroupNode::get(NullType<Namespaced>) {
	return m_contained.getNamespaced();
}

ModelSkin& Doom3GroupNode::get(NullType<ModelSkin>) {
	return m_contained.getModelSkin();
}

} // namespace entity
