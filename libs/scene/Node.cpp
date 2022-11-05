#include "Node.h"

#include "itransformnode.h"
#include "iscenegraph.h"
#include "debugging/debugging.h"
#include "InstanceWalkers.h"
#include "AABBAccumulateWalker.h"

namespace scene
{

Node::Node() :
	_state(eVisible),
	_isRoot(false),
	_id(getNewId()), // Get new auto-incremented ID
	_children(*this),
	_boundsChanged(true),
	_boundsMutex(false),
	_childBoundsChanged(true),
	_childBoundsMutex(false),
	_transformChanged(true),
	_transformMutex(false),
	_local2world(Matrix4::getIdentity()),
	_instantiated(false),
	_forceVisible(false),
    _renderEntity(nullptr),
    _renderState(RenderState::Active)
{
	// Each node is part of layer 0 by default
	_layers.insert(0);
}

Node::Node(const Node& other) :
	std::enable_shared_from_this<Node>(other),
	_state(other._state),
	_isRoot(other._isRoot),
	_id(getNewId()),	// ID is incremented on copy
	_children(*this),
	_boundsChanged(true),
	_boundsMutex(false),
	_childBoundsChanged(true),
	_childBoundsMutex(false),
	_local2world(other._local2world),
	_instantiated(false),
	_forceVisible(false),
	_layers(other._layers),
    _renderEntity(other._renderEntity),
    _renderState(other._renderState)
{}

scene::INodePtr Node::getSelf()
{
	return shared_from_this();
}

void Node::resetIds() {
	_maxNodeId = 0;
}

unsigned long Node::getNewId() {
	return ++_maxNodeId;
}

void Node::setSceneGraph(const GraphPtr& sceneGraph)
{
	_sceneGraph = sceneGraph;
}

bool Node::isRoot() const
{
	return _isRoot;
}

void Node::setIsRoot(bool isRoot)
{
	_isRoot = isRoot;
}

void Node::enable(unsigned int state)
{
    bool wasVisible = visible();

	_state |= state;

    // After setting a flag, this node may have changed to invisible
    if (wasVisible && _state != eVisible)
    {
        onVisibilityChanged(false);
    }
}

void Node::disable(unsigned int state)
{
    bool wasVisible = visible();

	_state &= ~state;

    // After clearing a flag, this node can only switch from invisible to visible
    if (!wasVisible && visible())
    {
        onVisibilityChanged(true);
    }
}

bool Node::checkStateFlag(unsigned int state) const
{
    return (_state & state) != 0;
}

bool Node::supportsStateFlag(unsigned int state) const
{
    return true;
}

bool Node::visible() const
{
	// Only instantiated nodes can be considered visible
	// The force visible flag is allowed to override the regular status
	return _instantiated && (_state == eVisible || _forceVisible);
}

bool Node::excluded() const
{
	return (_state & eExcluded) != 0;
}

void Node::addToLayer(int layerId)
{
	_layers.insert(layerId);
}

void Node::moveToLayer(int layerId)
{
	_layers.clear();
	_layers.insert(layerId);
}

void Node::removeFromLayer(int layerId)
{
	// Look up the layer ID and remove it from the list
	LayerList::iterator found = _layers.find(layerId);

	if (found != _layers.end()) {
		_layers.erase(found);

		// greebo: Make sure that every node is at least member of layer 0
		if (_layers.empty()) {
			_layers.insert(0);
		}
	}
}

const LayerList& Node::getLayers() const
{
	return _layers;
}

void Node::assignToLayers(const LayerList& newLayers)
{
	if (!newLayers.empty())
    {
        _layers = newLayers;
    }
}

void Node::addChildNode(const INodePtr& node)
{
	// Add the node to the TraversableNodeSet, this triggers an
	// Node::onChildAdded() event, where the parent of the new
	// child is set, among other things
	_children.append(node);
}

void Node::addChildNodeToFront(const INodePtr& node)
{
	// Add the node to the TraversableNodeSet at the front
	// This behaves the same as addChildNode(), triggering a
	// Node::onChildAdded() event, where the parent of the new
	// child is set, among other things
	_children.prepend(node);
}

void Node::removeChildNode(const INodePtr& node)
{
	// Remove the node from the TraversableNodeSet, this triggers an
	// Node::onChildRemoved() event
	_children.erase(node);

	// Clear out the parent, this is not done in onChildRemoved().
	node->setParent(INodePtr());
}

bool Node::hasChildNodes() const
{
	return !_children.empty();
}

void Node::removeAllChildNodes()
{
	_children.clear();
}

IMapRootNodePtr Node::getRootNode()
{
	if (getNodeType() == scene::INode::Type::MapRoot)
	{
		auto self = getSelf();
		assert(std::dynamic_pointer_cast<IMapRootNode>(self));
		return std::dynamic_pointer_cast<IMapRootNode>(self);
	}

	// Self is not a root node, start with the first ancestor, walking upwards
	for (auto node = getParent(); node; node = node->getParent())
	{
		if (node->getNodeType() == scene::INode::Type::MapRoot)
		{
			assert(std::dynamic_pointer_cast<IMapRootNode>(node));
			return std::dynamic_pointer_cast<IMapRootNode>(node);
		}
	}

	return IMapRootNodePtr();
}

void Node::traverse(NodeVisitor& visitor)
{
	// First, visit the node itself
	INodePtr self = getSelf();

	if (visitor.pre(self))
	{
		// The walker requested to descend the children of this node as well
		traverseChildren(visitor);
	}

	visitor.post(self);
}
void Node::traverseChildren(NodeVisitor& visitor) const
{
	if (!_children.empty())
	{
		_children.traverse(visitor);
	}
}

bool Node::foreachNode(const VisitorFunc& functor) const
{
	return _children.foreachNode(functor);
}

void Node::onChildAdded(const INodePtr& child)
{
	// Double-check the parent of this new child node
	if (child->getParent().get() != this)
	{
		child->setParent(shared_from_this());
	}

	// Pass down the RenderSystem to or children
	child->setRenderSystem(_renderSystem.lock());

	// greebo: The bounds most probably change when child nodes are added
	boundsChanged();

	if (!_instantiated) return;

	GraphPtr sceneGraph = _sceneGraph.lock();

	if (sceneGraph)
	{
		InstanceSubgraphWalker visitor(sceneGraph);
		child->traverse(visitor);
	}
}

void Node::onChildRemoved(const INodePtr& child)
{
	// Don't change the parent node of the new child on erase

	// greebo: The bounds are likely to change when child nodes are removed
	boundsChanged();

	if (!_instantiated) return;

	GraphPtr sceneGraph = _sceneGraph.lock();

	if (sceneGraph)
	{
		UninstanceSubgraphWalker visitor(*sceneGraph);
		child->traverse(visitor);
	}
}

void Node::onInsertIntoScene(IMapRootNode& root)
{
	_instantiated = true;

    // The node was 100% not visible before, check if it is now
    if (visible())
    {
        onVisibilityChanged(true);
    }

    connectUndoSystem(root.getUndoSystem());
}

void Node::onRemoveFromScene(IMapRootNode& root)
{
    disconnectUndoSystem(root.getUndoSystem());

    bool wasVisible = visible();

	_instantiated = false;

    // The node is 100% not visible after removing from the scene
    if (wasVisible)
    {
        onVisibilityChanged(false);
    }
}

void Node::connectUndoSystem(IUndoSystem& undoSystem)
{
    _children.connectUndoSystem(undoSystem);
}

void Node::disconnectUndoSystem(IUndoSystem& undoSystem)
{
    _children.disconnectUndoSystem(undoSystem);
}

TraversableNodeSet& Node::getTraversable() {
	return _children;
}

void Node::setParent(const INodePtr& parent) {
	_parent = parent;
}

scene::INodePtr Node::getParent() const {
	return _parent.lock();
}

void Node::getPathRecursively(Path& targetPath)
{
	INodePtr parent = getParent();

	assert(parent.get() != this); // avoid loopbacks

	if (parent != NULL) {
		std::dynamic_pointer_cast<Node>(parent)->getPathRecursively(targetPath);
	}

	// After passing the call to the parent, add self
	targetPath.push(shared_from_this());
}

Path Node::getPath()
{
	Path result;

	INodePtr parent = getParent();
	if (parent != NULL) {
		// We have a parent, walk up the ancestry
		std::dynamic_pointer_cast<Node>(parent)->getPathRecursively(result);
	}

	// Finally, add "self" to the path
	result.push(shared_from_this());

	return result;
}

const AABB& Node::worldAABB() const {
	evaluateBounds();
	return _bounds;
}

void Node::evaluateBounds() const
{
	if (_boundsChanged)
	{
		ASSERT_MESSAGE(!_boundsMutex, "re-entering bounds evaluation");
		_boundsMutex = true;

		_bounds = childBounds();

		_bounds.includeAABB(
			AABB::createFromOrientedAABBSafe(localAABB(), localToWorld())
		);

		_boundsMutex = false;
		_boundsChanged = false;

		// Now that our bounds are re-calculated, notify the scenegraph
		GraphPtr sceneGraph = _sceneGraph.lock();

		if (sceneGraph)
		{
			sceneGraph->nodeBoundsChanged(const_cast<Node*>(this)->shared_from_this());
		}
	}
}

const AABB& Node::childBounds() const {
	evaluateChildBounds();
	return _childBounds;
}

void Node::evaluateChildBounds() const {
	if (_childBoundsChanged) {
		ASSERT_MESSAGE(!_childBoundsMutex, "re-entering bounds evaluation");
		_childBoundsMutex = true;

		_childBounds = AABB();

		// Instantiate an AABB accumulator
		AABBAccumulateWalker accumulator(_childBounds);

		// greebo: traverse the children of this node
		traverseChildren(accumulator);

		_childBoundsMutex = false;
		_childBoundsChanged = false;
	}
}

void Node::boundsChanged() {
	_boundsChanged = true;
	_childBoundsChanged = true;

	INodePtr parent = _parent.lock();
	if (parent != NULL) {
		parent->boundsChanged();
	}

	// greebo: It's enough if only root nodes call the global scenegraph
	// as nodes are passing their calls up to their parents anyway
	if (_isRoot)
	{
		GraphPtr sceneGraph = _sceneGraph.lock();

		if (sceneGraph)
		{
			sceneGraph->boundsChanged();
		}
	}
}

const Matrix4& Node::localToWorld() const {
	evaluateTransform();
	return _local2world;
}

void Node::evaluateTransform() const {
	if (_transformChanged && !_transformMutex) {
		//ASSERT_MESSAGE(!_transformMutex, "re-entering transform evaluation");
		_transformMutex = true;

		INodePtr parent = _parent.lock();
		if (parent != NULL) {
			parent->boundsChanged();
		}

		_local2world = (parent != NULL) ? parent->localToWorld() : Matrix4::getIdentity();

		const ITransformNode* transformNode = dynamic_cast<const ITransformNode*>(this);

		if (transformNode != NULL) {
			_local2world.multiplyBy(transformNode->localToParent());
		}

		_transformMutex = false;
		_transformChanged = false;
	}
}

void Node::transformChangedLocal()
{
	_transformChanged = true;
	_transformMutex = false;
	_boundsChanged = true;
	_childBoundsChanged = true;
}

void Node::transformChanged()
{
	// First, notify ourselves
	transformChangedLocal();

	// Next, traverse the children and notify them
	_children.foreachNode([this] (const scene::INodePtr& child)->bool
	{
		child->transformChangedLocal();
		return true;
	});

	boundsChanged();
}

RenderSystemPtr Node::getRenderSystem() const
{
	return _renderSystem.lock();
}

void Node::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	_renderSystem = renderSystem;

	if (_children.empty()) return;

	// Propagate this call to all children
	_children.setRenderSystem(renderSystem);
}

void Node::setForcedVisibility(bool forceVisible, bool includeChildren)
{
    bool wasVisible = visible();

	_forceVisible = forceVisible;

    bool isVisible = visible();

    if (wasVisible ^ isVisible)
    {
        onVisibilityChanged(isVisible);
    }

	if (includeChildren)
	{
		_children.foreachNode([&](const INodePtr& node)
		{
			node->setForcedVisibility(forceVisible, includeChildren);
			return true;
		});
	}
}

bool Node::isForcedVisible() const
{
	return _forceVisible;
}

INode::RenderState Node::getRenderState() const
{
    return _renderState;
}

void Node::setRenderState(RenderState state)
{
    if (state != _renderState)
    {
        _renderState = state;
        onRenderStateChanged();
    }
}

unsigned long Node::_maxNodeId = 0;

} // namespace scene
