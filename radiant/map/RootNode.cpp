#include "RootNode.h"

namespace map {

RootNode::RootNode(const std::string& name) : 
	_name(name)
{
	// Apply root status to this node
	setIsRoot(true);
	// Attach ourselves as scene::Traversable::Observer 
	// to the TraversableNodeset >> triggers instantiate calls.
	attachTraverseObserver(this);

	GlobalUndoSystem().trackerAttach(m_changeTracker);
}

RootNode::~RootNode() {
	// Override the default release() method
	GlobalUndoSystem().trackerDetach(m_changeTracker);
	
	// Remove ourselves as observer from the TraversableNodeSet
	detachTraverseObserver(this);
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
		Node::instanceAttach(path_find_mapfile(path.begin(), path.end()));
	}
}

void RootNode::instanceDetach(const scene::Path& path) {
	if (--m_instanceCounter.m_count == 0) {
		Node::instanceDetach(path_find_mapfile(path.begin(), path.end()));
	}
}

scene::INodePtr RootNode::clone() const {
	return scene::INodePtr(new RootNode(*this));
}

void RootNode::instantiate(const scene::Path& path) {
	Node::instantiate(path);
	instanceAttach(path);
}

void RootNode::uninstantiate(const scene::Path& path) {
	instanceDetach(path);
	Node::uninstantiate(path);
}

} // namespace map
