#include "LightNode.h"

// --------- LightNode implementation ------------------------------------

LightNode::LightNode(IEntityClassPtr eclass) :
	m_contained(eclass, *this, InstanceSet::TransformChangedCaller(m_instances), InstanceSet::BoundsChangedCaller(m_instances), InstanceSetEvaluateTransform<LightInstance>::Caller(m_instances))
{
	construct();
}

LightNode::LightNode(const LightNode& other) :
	scene::Node(other),
	scene::Instantiable(other),
	scene::Cloneable(other),
	scene::Traversable::Observer(other),
	Nameable(other),
	Snappable(other),
	Editable(other),
	TransformNode(other),
	scene::Traversable(other),
	EntityNode(other),
	Namespaced(other),
	m_contained(other.m_contained, *this, InstanceSet::TransformChangedCaller(m_instances), InstanceSet::BoundsChangedCaller(m_instances), InstanceSetEvaluateTransform<LightInstance>::Caller(m_instances))
{
	construct();
}

LightNode::~LightNode() {
	destroy();
}

void LightNode::construct() {
	// Attach this node as scene::Traversable::Observer to the contained Light class
	m_contained.attach(this);
}

void LightNode::destroy() {
	m_contained.detach(this);
}

void LightNode::insert(scene::INodePtr node) {
	m_contained.getTraversable().insert(node);
}

void LightNode::erase(scene::INodePtr node) {
	m_contained.getTraversable().erase(node);
}

void LightNode::traverse(const Walker& walker) {
	m_contained.getTraversable().traverse(walker);
}

bool LightNode::empty() const {
	return m_contained.getTraversable().empty();
}

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

void LightNode::insertChild(scene::INodePtr child) {
	m_instances.insertChild(child);
}

void LightNode::eraseChild(scene::INodePtr child) {
	m_instances.eraseChild(child);
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
