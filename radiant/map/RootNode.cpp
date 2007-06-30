#include "RootNode.h"

namespace map {

RootNode::RootNode(const std::string& name) : 
	_name(name)
{
	// Apply root status to this node
	setIsRoot(true);
	// Attach the InstanceSet as scene::Traversable::Observer 
	// to the TraversableNodeset >> triggers instancing.
	m_traverse.attach(&m_instances);

	GlobalUndoSystem().trackerAttach(m_changeTracker);
}

RootNode::~RootNode() {
	// Override the default release() method
	GlobalUndoSystem().trackerDetach(m_changeTracker);
	
	// Remove the observer InstanceSet from the TraversableNodeSet
	m_traverse.detach(&m_instances);
}

// scene::Traversable Implementation
void RootNode::insert(scene::INodePtr node) {
	m_traverse.insert(node);
}
void RootNode::erase(scene::INodePtr node) {
	m_traverse.erase(node);	
}
void RootNode::traverse(const Walker& walker) {
	m_traverse.traverse(walker);
}
bool RootNode::empty() const {
	return m_traverse.empty();
}

// TransformNode implementation
const Matrix4& RootNode::localToParent() const {
	return m_transform.localToParent();
}

// MapFile implementation
void RootNode::save() {
	m_changeTracker.save();
}

bool RootNode::saved() const {
	return m_changeTracker.saved();
}

void RootNode::changed() {
	m_changeTracker.changed();
}

void RootNode::setChangedCallback(const Callback& changed) {
	m_changeTracker.setChangedCallback(changed);
}

std::size_t RootNode::changes() const {
	return m_changeTracker.changes();
}

std::string RootNode::name() const {
	return _name;
}

void RootNode::instanceAttach(const scene::Path& path) {
	if (++m_instanceCounter.m_count == 1) {
		m_traverse.instanceAttach(path_find_mapfile(path.begin(), path.end()));
	}
}

void RootNode::instanceDetach(const scene::Path& path) {
	if (--m_instanceCounter.m_count == 0) {
		m_traverse.instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

scene::INodePtr RootNode::clone() const {
	return scene::INodePtr(new RootNode(*this));
}

scene::Instance* RootNode::create(const scene::Path& path, scene::Instance* parent) {
	return new SelectableInstance(path, parent);
}

void RootNode::forEachInstance(const scene::Instantiable::Visitor& visitor) {
	m_instances.forEachInstance(visitor);
}

void RootNode::insert(const scene::Path& path, scene::Instance* instance) {
	m_instances.insert(path, instance);
	instanceAttach(path);
}

scene::Instance* RootNode::erase(const scene::Path& path) {
	instanceDetach(path);
	return m_instances.erase(path);
}

} // namespace map
