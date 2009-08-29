#include "Node.h"

#include "scenelib.h"
#include "InstanceWalkers.h"

namespace scene {

	namespace {

		/**
		 * greebo: This walker is used to traverse the children of
		 *         a given node and accumulate their values of worldAABB().
		 * 
		 * Note: This walker's pre() method always returns false, so only
		 *       the first order children are visited.
		 */
		class AABBAccumulateWalker : 
			public scene::NodeVisitor
		{
			AABB& _aabb;
		public:
			AABBAccumulateWalker(AABB& aabb) : 
				_aabb(aabb)
			{}
			
			virtual bool pre(const INodePtr& node) {
				_aabb.includeAABB(node->worldAABB());
				// Don't traverse the children
				return false; 
			}
		};

		class TransformChangedWalker : 
			public NodeVisitor
		{
		public:
			virtual bool pre(const INodePtr& node) {
				boost::static_pointer_cast<Node>(node)->transformChangedLocal();
				return true;
			}
		};

	} // namespace

Node::Node() :
	_state(eVisible),
	_isRoot(false),
	_id(getNewId()), // Get new auto-incremented ID
	_boundsChanged(true),
	_boundsMutex(false),
	_childBoundsChanged(true),
	_childBoundsMutex(false),
	_transformChanged(true),
	_transformMutex(false),
	_local2world(Matrix4::getIdentity()),
	_instantiated(false)
{
	// Each node is part of layer 0 by default
	_layers.insert(0);
}

Node::Node(const Node& other) :
	INode(other),
	Traversable::Observer(other),
	_state(other._state),
	_isRoot(other._isRoot),
	_id(getNewId()),	// ID is incremented on copy
	_boundsChanged(true),
	_boundsMutex(false),
	_childBoundsChanged(true),
	_childBoundsMutex(false),
	_local2world(other._local2world),
	_instantiated(false),
	_layers(other._layers)
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

bool Node::isRoot() const {
	return _isRoot;
}

void Node::setIsRoot(bool isRoot) {
	_isRoot = isRoot;
}

void Node::enable(unsigned int state) {
	_state |= state;
}

void Node::disable(unsigned int state) {
	_state &= ~state;
}

bool Node::visible() const {
	return _state == eVisible;
}

bool Node::excluded() const {
	return (_state & eExcluded) != 0;
}

void Node::addToLayer(int layerId) {
	_layers.insert(layerId);
}

void Node::moveToLayer(int layerId) {
	_layers.clear();
	_layers.insert(layerId);
}

void Node::removeFromLayer(int layerId) {
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

LayerList Node::getLayers() const {
	return _layers;
}

void Node::addChildNode(const INodePtr& node) {
	// Add the node to the TraversableNodeSet, this triggers an 
	// Node::onTraversableInsert() event
	_children.insert(node);

	// Set the parent of this new node
	node->setParent(shared_from_this());

	// greebo: The bounds most probably change when child nodes are added
	boundsChanged();
}

void Node::removeChildNode(const INodePtr& node) {
	// Remove the node from the TraversableNodeSet, this triggers an 
	// Node::onTraversableErase() event
	_children.erase(node);

	// Set the parent of this node to NULL
	node->setParent(scene::INodePtr());

	// greebo: The bounds are likely to change when child nodes are removed
	boundsChanged();
}

bool Node::hasChildNodes() const {
	return !_children.empty();
}

void Node::removeAllChildNodes() {
	_children.clear();
}

void Node::traverse(NodeVisitor& visitor) const
{
	_children.traverse(visitor);
}

// traverse observer
// greebo: This gets called as soon as a scene::Node gets inserted into
// the oberved Traversable. This triggers an instantiation call and ensures
// that each inserted node is also instantiated.
void Node::onTraversableInsert(const INodePtr& child) {
	if (!_instantiated) return;

	Path parentPath = getPath();
	
	child->setParent(parentPath.top());
	
	InstanceSubgraphWalker visitor(parentPath);
	Node_traverseSubgraph(child, visitor);

	child->boundsChanged();
}

void Node::onTraversableErase(const INodePtr& child) {
	if (!_instantiated) return;

	Path childPath = getPath();
	
	UninstanceSubgraphWalker visitor(childPath);
	Node_traverseSubgraph(child, visitor);

	child->setParent(scene::INodePtr());
	child->boundsChanged();
}

void Node::instantiate(const scene::Path& path) {
	_instantiated = true;
}

void Node::uninstantiate(const scene::Path& path) {
	_instantiated = false;
}

void Node::attachTraverseObserver(scene::Traversable::Observer* observer) {
	_children.attach(observer);
}

void Node::detachTraverseObserver(scene::Traversable::Observer* observer) {
	_children.detach(observer);
}

void Node::instanceAttach(MapFile* mapfile) {
	_children.instanceAttach(mapfile);
}

void Node::instanceDetach(MapFile* mapfile) {
	_children.instanceDetach(mapfile);
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

void Node::getPathRecursively(scene::Path& targetPath) {
	scene::INodePtr parent = getParent();

	assert(parent.get() != this); // avoid loopbacks

	if (parent != NULL) {
		boost::static_pointer_cast<Node>(parent)->getPathRecursively(targetPath);
	}

	// After passing the call to the parent, add self
	targetPath.push(shared_from_this());
}

scene::Path Node::getPath()
{
	scene::Path result;

	scene::INodePtr parent = getParent();
	if (parent != NULL) {
		// We have a parent, walk up the ancestry
		boost::static_pointer_cast<Node>(parent)->getPathRecursively(result);
	}

	// Finally, add "self" to the path
	result.push(shared_from_this());

	return result;
}

const AABB& Node::worldAABB() const {
	evaluateBounds();
	return _bounds;
}

void Node::evaluateBounds() const {
	if(_boundsChanged) {
		ASSERT_MESSAGE(!_boundsMutex, "re-entering bounds evaluation");
		_boundsMutex = true;

		_bounds = childBounds();

		const Bounded* bounded = dynamic_cast<const Bounded*>(this);

		if (bounded != NULL) {
			_bounds.includeAABB(
			    aabb_for_oriented_aabb_safe(bounded->localAABB(), localToWorld())
			);
		}

		_boundsMutex = false;
		_boundsChanged = false;
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
		const_cast<Node*>(this)->traverse(accumulator);
		
		_childBoundsMutex = false;
		_childBoundsChanged = false;
	}
}

void Node::boundsChanged() {
	_boundsChanged = true;
	_childBoundsChanged = true;

	scene::INodePtr parent = _parent.lock();
	if (parent != NULL) {
		parent->boundsChanged();
	}

	if (_isRoot)
	{
		// greebo: It's enough if only root nodes call the global scenegraph
		// as nodes are passing their calls up to their parents anyway
		GlobalSceneGraph().boundsChanged();
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

		scene::INodePtr parent = _parent.lock();
		if (parent != NULL) {
			parent->boundsChanged();
		}

		_local2world = (parent != NULL) ? parent->localToWorld() : Matrix4::getIdentity();

		const TransformNode* transformNode = dynamic_cast<const TransformNode*>(this);

		if (transformNode != NULL) {
			_local2world.multiplyBy(transformNode->localToParent());
		}

		_transformMutex = false;
		_transformChanged = false;
	}
}

void Node::transformChangedLocal() {
	_transformChanged = true;
	_transformMutex = false;
	_boundsChanged = true;
	_childBoundsChanged = true;

	_transformChangedCallback();
}

void Node::transformChanged() {
	// First, notify ourselves
	transformChangedLocal();

	// Next, traverse the children and notify them
	TransformChangedWalker walker;
	traverse(walker);
	
	boundsChanged();
}

void Node::setTransformChangedCallback(const Callback& callback) {
	_transformChangedCallback = callback;
}

unsigned long Node::_maxNodeId = 0;

} // namespace scene
