#include "LightNode.h"

// --------- LightNode implementation ------------------------------------

void LightNode::construct() {
	if(g_lightType == LIGHTTYPE_DOOM3) {
		m_contained.attach(this);
	}
}

void LightNode::destroy() {
	if(g_lightType == LIGHTTYPE_DOOM3) {
		m_contained.detach(this);
	}
}

scene::Traversable& LightNode::get(NullType<scene::Traversable>) {
	return m_contained.getTraversable();
}

Editable& LightNode::get(NullType<Editable>) {
	return m_contained;
}

Snappable& LightNode::get(NullType<Snappable>) {
	return m_contained;
}

TransformNode& LightNode::get(NullType<TransformNode>) {
	return m_contained.getTransformNode();
}

Entity& LightNode::get(NullType<Entity>) {
	return m_contained.getEntity();
}

Nameable& LightNode::get(NullType<Nameable>) {
	return m_contained.getNameable();
}

Namespaced& LightNode::get(NullType<Namespaced>) {
	return m_contained.getNamespaced();
}

scene::Node& LightNode::node() {
	return m_node;
}

scene::Node& LightNode::clone() const {
	return (new LightNode(*this))->node();
}

void LightNode::insert(scene::Node& child) {
	m_instances.insert(child);
}

void LightNode::erase(scene::Node& child) {
	m_instances.erase(child);
}

scene::Instance* LightNode::create(const scene::Path& path, scene::Instance* parent) {
	return new LightInstance(path, parent, m_contained);
}

void LightNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

void LightNode::insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(observer, path, instance);
}

scene::Instance* LightNode::erase(scene::Instantiable::Observer* observer, const scene::Path& path) {
	return m_instances.erase(observer, path);
}
