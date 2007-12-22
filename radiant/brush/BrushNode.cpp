#include "BrushNode.h"

#include "BrushInstance.h"

// Constructor
BrushNode::BrushNode() :
	BrushTokenImporter(m_brush),
	BrushTokenExporter(m_brush),
	m_brush(*this, InstanceSetEvaluateTransform<BrushInstance>::Caller(m_instances), InstanceSet::BoundsChangedCaller(m_instances))
{}

// Copy Constructor
BrushNode::BrushNode(const BrushNode& other) :
	scene::Node(other),
	scene::Instantiable(other),
	scene::Cloneable(other),
	Nameable(other),
	Snappable(other),
	TransformNode(other),
	BrushDoom3(other),
	BrushTokenImporter(m_brush),
	BrushTokenExporter(m_brush),
	IBrushNode(other),
	m_brush(other.m_brush, *this, InstanceSetEvaluateTransform<BrushInstance>::Caller(m_instances), InstanceSet::BoundsChangedCaller(m_instances))
{}

// Snappable implementation
void BrushNode::snapto(float snap) {
	m_brush.snapto(snap);
}

// TransformNode implementation
const Matrix4& BrushNode::localToParent() const {
	return m_brush.localToParent();
}

// IBrushNode implementation
Brush& BrushNode::getBrush() {
	return m_brush;
}

void BrushNode::translateDoom3Brush(const Vector3& translation) {
	m_brush.translateDoom3Brush(translation);
}

scene::INodePtr BrushNode::clone() const {
	return scene::INodePtr(new BrushNode(*this));
}

scene::Instance* BrushNode::create(const scene::Path& path, scene::Instance* parent) {
	return new BrushInstance(path, parent, m_brush);
}

void BrushNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

void BrushNode::insert(const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(path, instance);
}

scene::Instance* BrushNode::erase(const scene::Path& path) {
	return m_instances.erase(path);
}
