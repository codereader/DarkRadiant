#include "RootNode.h"

#include "inode.h"

namespace map
{

RootNode::RootNode(const std::string& name) :
	_name(name),
	_instanceCounter(0)
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

void RootNode::onInsertIntoScene(IMapFileChangeTracker* map)
{
	if (++_instanceCounter == 1)
	{
		Node::onInsertIntoScene(map);
	}
}

void RootNode::onRemoveFromScene(IMapFileChangeTracker* map)
{
	if (--_instanceCounter == 0)
	{
		Node::onRemoveFromScene(map);
	}
}

scene::INodePtr RootNode::clone() const {
	return scene::INodePtr(new RootNode(*this));
}

void RootNode::onInsertIntoScene(const IMapRootNode& root)
{
	Node::onInsertIntoScene(root);

	onInsertIntoScene(scene::findMapFile(getSelf()));
}

void RootNode::onRemoveFromScene(const IMapRootNode& root)
{
	onRemoveFromScene(scene::findMapFile(getSelf()));

	Node::onRemoveFromScene(root);
}

} // namespace map
