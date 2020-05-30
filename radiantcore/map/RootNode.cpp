#include "RootNode.h"

#include "inode.h"

namespace map
{

RootNode::RootNode(const std::string& name) :
	_name(name)
{
	// Apply root status to this node
	setIsRoot(true);

	GlobalUndoSystem().attachTracker(*this);

	// Create a new namespace
	_namespace = GlobalNamespaceFactory().createNamespace();
	assert(_namespace);

    _targetManager = GlobalEntityModule().createTargetManager();
    assert(_targetManager);

	_selectionGroupManager = GlobalSelectionGroupModule().createSelectionGroupManager();
	assert(_selectionGroupManager);

	_selectionSetManager = GlobalSelectionSetModule().createSelectionSetManager();
	assert(_selectionSetManager);

	_layerManager = GlobalLayerModule().createLayerManager();
	assert(_layerManager);
}

RootNode::~RootNode()
{
	GlobalUndoSystem().detachTracker(*this);

	// Remove all child nodes to trigger their destruction
	removeAllChildNodes();
}

const INamespacePtr& RootNode::getNamespace()
{
	return _namespace;
}

IMapFileChangeTracker& RootNode::getUndoChangeTracker() 
{
    return *this;
}

ITargetManager& RootNode::getTargetManager()
{
    return *_targetManager;
}

selection::ISelectionGroupManager& RootNode::getSelectionGroupManager()
{
	return *_selectionGroupManager;
}

selection::ISelectionSetManager& RootNode::getSelectionSetManager()
{
	return *_selectionSetManager;
}

scene::ILayerManager& RootNode::getLayerManager()
{
	return *_layerManager;
}

std::string RootNode::name() const 
{
	return _name;
}

scene::INode::Type RootNode::getNodeType() const
{
	return Type::MapRoot;
}

void RootNode::setName(const std::string& name)
{
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
