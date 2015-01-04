#include "RootNode.h"

#include "inode.h"

namespace map
{

RootNode::RootNode(const std::string& name) :
	_name(name)
{
	// Apply root status to this node
	setIsRoot(true);

	GlobalUndoSystem().attachTracker(_changeTracker);

	// Create a new namespace
	_namespace = GlobalNamespaceFactory().createNamespace();
	assert(_namespace != NULL);
}

RootNode::~RootNode()
{
	GlobalUndoSystem().detachTracker(_changeTracker);

	// Remove all child nodes to trigger their destruction
	removeAllChildNodes();
}

const INamespacePtr& RootNode::getNamespace()
{
	return _namespace;
}

IMapFileChangeTracker& RootNode::getUndoChangeTracker() 
{
    return _changeTracker;
}

// MapFile implementation
void RootNode::save() {
	_changeTracker.save();
}

bool RootNode::saved() const {
	return _changeTracker.saved();
}

void RootNode::changed() {
	_changeTracker.changed();
}

void RootNode::setChangedCallback(const std::function<void()>& changed) {
	_changeTracker.setChangedCallback(changed);
}

std::size_t RootNode::changes() const {
	return _changeTracker.changes();
}

std::string RootNode::name() const {
	return _name;
}

scene::INode::Type RootNode::getNodeType() const
{
	return Type::MapRoot;
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

void RootNode::onInsertIntoScene(IMapRootNode& root)
{
	Node::onInsertIntoScene(root);

    // A RootNode supports child entities, so connect
    // the Node's TraversableNodeSet to the UndoSystem
    Node::connectUndoSystem(root.getUndoChangeTracker());
}

void RootNode::onRemoveFromScene(IMapRootNode& root)
{
    // A RootNode supports child entities, so disconnect
    // the Node's TraversableNodeSet to the UndoSystem
    Node::disconnectUndoSystem(root.getUndoChangeTracker());

	Node::onRemoveFromScene(root);
}

} // namespace map
