#include "BrushNode.h"

#include "BrushInstance.h"

// Constructor
BrushNode::BrushNode() :
	scene::Node(this, StaticTypeCasts::instance().get()),
	m_brush(*this, InstanceSetEvaluateTransform<BrushInstance>::Caller(m_instances), InstanceSet::BoundsChangedCaller(m_instances)),
	m_mapImporter(m_brush),
	m_mapExporter(m_brush)
{}

// Copy Constructor
BrushNode::BrushNode(const BrushNode& other) :
	scene::Node(this, StaticTypeCasts::instance().get()),
	scene::Instantiable(other),
	scene::Cloneable(other),
	Nameable(other),
	m_brush(other.m_brush, *this, InstanceSetEvaluateTransform<BrushInstance>::Caller(m_instances), InstanceSet::BoundsChangedCaller(m_instances)),
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

BrushDoom3& BrushNode::get(NullType<BrushDoom3>) {
	return m_brush;
}

scene::Node& BrushNode::node() {
	return *this;
}
 
scene::Node& BrushNode::clone() const {
	return (new BrushNode(*this))->node();
}

scene::Instance* BrushNode::create(const scene::Path& path, scene::Instance* parent) {
	return new BrushInstance(path, parent, m_brush);
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
