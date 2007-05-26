#include "BrushNode.h"

#include "BrushInstance.h"

// Constructor
BrushNode::BrushNode() :
	m_brush(*this, InstanceSetEvaluateTransform<BrushInstance>::Caller(m_instances), InstanceSet::BoundsChangedCaller(m_instances)),
	m_mapImporter(m_brush),
	m_mapExporter(m_brush)
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
	MapImporter(other),
	MapExporter(other),
	IBrushNode(other),
	m_brush(other.m_brush, *this, InstanceSetEvaluateTransform<BrushInstance>::Caller(m_instances), InstanceSet::BoundsChangedCaller(m_instances)),
	m_mapImporter(m_brush),
	m_mapExporter(m_brush)
{}

// MapImporter implementation
bool BrushNode::importTokens(Tokeniser& tokeniser) {
	return m_mapImporter.importTokens(tokeniser);
}

void BrushNode::exportTokens(std::ostream& os) const {
	m_mapExporter.exportTokens(os);
}

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

void BrushNode::insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(observer, path, instance);
}

scene::Instance* BrushNode::erase(scene::Instantiable::Observer* observer, const scene::Path& path) {
	return m_instances.erase(observer, path);
}
