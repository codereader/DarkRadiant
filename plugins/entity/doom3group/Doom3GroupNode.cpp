#include "Doom3GroupNode.h"

#include "Doom3GroupInstance.h"

namespace entity {

Doom3GroupNode::Doom3GroupNode(IEntityClassPtr eclass) :
	scene::Node(this, StaticTypeCasts::instance().get()),
	m_contained(
		eclass,
		*this,
		InstanceSet::TransformChangedCaller(m_instances),
		InstanceSet::BoundsChangedCaller(m_instances),
		InstanceSetEvaluateTransform<Doom3GroupInstance>::Caller(m_instances)
	) 
{
	construct();
}

Doom3GroupNode::Doom3GroupNode(const Doom3GroupNode& other) :
	scene::Node(this, StaticTypeCasts::instance().get()),
	scene::Instantiable(other),
	scene::Cloneable(other),
	scene::Traversable::Observer(other),
	scene::GroupNode(other),
	Nameable(other),
	m_contained(
		other.m_contained,
		*this,
		InstanceSet::TransformChangedCaller(m_instances),
		InstanceSet::BoundsChangedCaller(m_instances),
		InstanceSetEvaluateTransform<Doom3GroupInstance>::Caller(m_instances)
	) 
{
	// Attach this node to the contained Doom3Group as observer
	construct();
}

void Doom3GroupNode::addOriginToChildren() {
	m_contained.addOriginToChildren();
}

void Doom3GroupNode::removeOriginFromChildren() {
	m_contained.removeOriginFromChildren();
}

std::string Doom3GroupNode::name() const {
	return m_contained.getNameable().name();
}

void Doom3GroupNode::attach(const NameCallback& callback) {
	m_contained.getNameable().attach(callback);
}

void Doom3GroupNode::detach(const NameCallback& callback) {
	m_contained.getNameable().detach(callback);
}

void Doom3GroupNode::construct() {
	// Attach this node to the contained Doom3Group as observer
	m_contained.attach(this);
}

void Doom3GroupNode::destroy() {
	// Detach this node to the contained Doom3Group as observer
	m_contained.detach(this);
}

Doom3GroupNode::~Doom3GroupNode() {
	// Detach this node to the contained Doom3Group as observer
	destroy();
}

scene::Node& Doom3GroupNode::node() {
	return *this;
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

Namespaced& Doom3GroupNode::get(NullType<Namespaced>) {
	return m_contained.getNamespaced();
}

ModelSkin& Doom3GroupNode::get(NullType<ModelSkin>) {
	return m_contained.getModelSkin();
}

} // namespace entity
