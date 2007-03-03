#include "BrushNode.h"

#include "BrushInstance.h"

// Constructor
BrushNode::BrushNode() :
	m_node(this, this, StaticTypeCasts::instance().get()),
	m_brush(m_node, InstanceSetEvaluateTransform<BrushInstance>::Caller(m_instances), InstanceSet::BoundsChangedCaller(m_instances)),
	m_mapImporter(m_brush),
	m_mapExporter(m_brush)
{}

// Copy Constructor
BrushNode::BrushNode(const BrushNode& other) :
	scene::Node::Symbiot(other),
	scene::Instantiable(other),
	scene::Cloneable(other),
	m_node(this, this, StaticTypeCasts::instance().get()),
	m_brush(other.m_brush, m_node, InstanceSetEvaluateTransform<BrushInstance>::Caller(m_instances), InstanceSet::BoundsChangedCaller(m_instances)),
	m_mapImporter(m_brush),
	m_mapExporter(m_brush)
{}

// Typecast functions
Snappable& BrushNode::get(NullType<Snappable>)	{
	return m_brush;
}

TransformNode& BrushNode::get(NullType<TransformNode>) {
	return m_brush;
}

Brush& BrushNode::get(NullType<Brush>)	{
	return m_brush;
}

MapImporter& BrushNode::get(NullType<MapImporter>)	{
	return m_mapImporter;
}

MapExporter& BrushNode::get(NullType<MapExporter>)	{
	return m_mapExporter;
}

Nameable& BrushNode::get(NullType<Nameable>) {
	return m_brush;
}

scene::Node& BrushNode::node() {
	return m_node;
}
 
scene::Node& BrushNode::clone() const {
	return (new BrushNode(*this))->node();
}

scene::Instance* BrushNode::create(const scene::Path& path, scene::Instance* parent) {
	return new BrushInstance(path, parent, m_brush);
}

void BrushNode::release() {
	delete this;
}

void BrushNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

void BrushNode::insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(observer, path, instance);
}

scene::Instance* BrushNode::erase(scene::Instantiable::Observer* observer, const scene::Path& path) {
	return m_instances.erase(observer, path);
}
