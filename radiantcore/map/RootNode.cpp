#include "RootNode.h"

#include "inode.h"

namespace map
{

RootNode::RootNode(const std::string& name) :
	_name(name)
{
	// Apply root status to this node
	setIsRoot(true);

	// Create a new namespace
	_namespace = GlobalNamespaceFactory().createNamespace();
	assert(_namespace);

    _targetManager = GlobalEntityModule().createTargetManager();
    assert(_targetManager);

	_selectionGroupManager = GlobalSelectionGroupModule().createSelectionGroupManager();
	assert(_selectionGroupManager);

	_selectionSetManager = GlobalSelectionSetModule().createSelectionSetManager();
	assert(_selectionSetManager);

	_layerManager = GlobalLayerModule().createLayerManager(*this);
	assert(_layerManager);

    _undoSystem = GlobalUndoSystemFactory().createUndoSystem();
    assert(_undoSystem);

    _undoEventHandler = _undoSystem->signal_undoEvent().connect(
        sigc::mem_fun(this, &RootNode::onUndoEvent)
    );
}

RootNode::~RootNode()
{
    _undoEventHandler.disconnect();

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

IUndoSystem& RootNode::getUndoSystem()
{
    return *_undoSystem;
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

void RootNode::onFiltersChanged()
{
    // Recursively notify the whole tree
    foreachNode([](const scene::INodePtr& node)
    {
        node->onFiltersChanged();
        return true;
    });
}

} // namespace map
