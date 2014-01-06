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

INamespacePtr RootNode::getNamespace() {
	return _namespace;
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

void RootNode::setChangedCallback(const boost::function<void()>& changed) {
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

void RootNode::instanceAttach(MapFile* map)
{
	if (++_instanceCounter == 1)
	{
		Node::instanceAttach(map);
	}
}

void RootNode::instanceDetach(MapFile* map)
{
	if (--_instanceCounter == 0)
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
