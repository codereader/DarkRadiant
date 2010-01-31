#include "RootNode.h"

namespace map {

RootNode::RootNode(const std::string& name) : 
	_name(name)
{
	// Apply root status to this node
	setIsRoot(true);

	GlobalUndoSystem().trackerAttach(m_changeTracker);

	// Create a new namespace
	_namespace = GlobalNamespaceFactory().createNamespace();
	assert(_namespace != NULL);
}

RootNode::~RootNode() {
	// Override the default release() method
	GlobalUndoSystem().trackerDetach(m_changeTracker);
	
	// Remove all child nodes to trigger their destruction
	removeAllChildNodes();
}

INamespacePtr RootNode::getNamespace() {
	return _namespace;
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

void RootNode::setName(const std::string& name) {
	_name = name;
}

void RootNode::onChildAdded(const scene::INodePtr& child)
{
	// Insert this node into our namespace
	_namespace->connect(child);

	Node::onChildAdded(child);
}

void RootNode::onChildRemoved(const scene::INodePtr& child)
{
	// Detach the node from our namespace
	_namespace->disconnect(child);

	Node::onChildRemoved(child);
}

void RootNode::instanceAttach(MapFile* map)
{
	if (++m_instanceCounter.m_count == 1) {
		Node::instanceAttach(map);
	}
}

void RootNode::instanceDetach(MapFile* map)
{
	if (--m_instanceCounter.m_count == 0)
	{
		Node::instanceDetach(map);
	}
}

scene::INodePtr RootNode::clone() const {
	return scene::INodePtr(new RootNode(*this));
}

void RootNode::onInsertIntoScene()
{
	Node::onInsertIntoScene();

	instanceAttach(scene::findMapFile(getSelf()));
}

void RootNode::onRemoveFromScene()
{
	instanceDetach(scene::findMapFile(getSelf()));

	Node::onRemoveFromScene();
}

} // namespace map
