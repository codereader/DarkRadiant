#pragma once

#include "inode.h"
#include "ipath.h"
#include "irender.h"
#include <list>
#include "TraversableNodeSet.h"
#include "math/AABB.h"
#include "math/Matrix4.h"
#include "generic/callback.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

namespace scene
{

class Graph;
typedef boost::weak_ptr<Graph> GraphWeakPtr;

/// Main implementation of INode
class Node :
	public virtual INode,
	public boost::enable_shared_from_this<Node>
{
public:
	enum {
		eVisible = 0,
		eHidden = 1 << 0,    // manually hidden by the user
		eFiltered = 1 << 1,  // excluded due to filter settings
		eExcluded = 1 << 2,  // excluded due to regioning
		eLayered = 1 << 3    // invisible at the current layer settings
	};

private:
	unsigned int _state;
	bool _isRoot;
	unsigned long _id;

	// Auto-incrementing ID (contains the largest ID in use)
	static unsigned long _maxNodeId;

	TraversableNodeSet _children;

	// A weak reference to the parent node
	INodeWeakPtr _parent;

	mutable AABB _bounds;
	mutable AABB _childBounds;
	mutable bool _boundsChanged;
	mutable bool _boundsMutex;
	mutable bool _childBoundsChanged;
	mutable bool _childBoundsMutex;
	mutable bool _transformChanged;
	mutable bool _transformMutex;
	Callback _transformChangedCallback;

	mutable Matrix4 _local2world;

	// Is true when the node is part of the scenegraph
	bool _instantiated;

	// The list of layers this object is associated to
	LayerList _layers;

protected:
	// If this node is attached to a parent entity, this is the reference to it
	IRenderEntityPtr _renderEntity;

	// The render system for passing down the child hierarchy later on
	RenderSystemWeakPtr _renderSystem;

	// The scene::Graph we're belonging to (weak reference to avoid circular references)
	GraphWeakPtr _sceneGraph;

public:
	Node();
	Node(const Node& other);

	static void resetIds();
	static unsigned long getNewId();

    // Default name for generic nodes
    std::string name() const { return "node"; }

	void setSceneGraph(const GraphPtr& sceneGraph);

	bool isRoot() const;
	void setIsRoot(bool isRoot);

	void enable(unsigned int state);
	void disable(unsigned int state);

	bool visible() const;

	bool excluded() const;

	// Layered implementation
	virtual void addToLayer(int layerId);
    virtual void removeFromLayer(int layerId);
	virtual void moveToLayer(int layerId);
    virtual LayerList getLayers() const;
	virtual void assignToLayers(const LayerList& newLayers);

	virtual void addChildNode(const INodePtr& node);
	virtual void removeChildNode(const INodePtr& node);
	virtual bool hasChildNodes() const;

	virtual void traverse(NodeVisitor& visitor);
	virtual void traverseChildren(NodeVisitor& visitor) const;
	virtual bool foreachNode(const VisitorFunc& functor) const;

	virtual void setParent(const INodePtr& parent);
	virtual scene::INodePtr getParent() const;

	const AABB& worldAABB() const;

	const AABB& childBounds() const;

	void boundsChanged();

	/**
	 * Return the filtered status of this Instance.
	 */
	virtual bool isFiltered() const {
		return (_state & eFiltered) != 0;
	}

	/**
	 * Set the filtered status of this Node. Setting filtered to true will
	 * prevent the node from being rendered.
	 */
	virtual void setFiltered(bool filtered) {
		if (filtered) {
			_state |= eFiltered;
		}
		else {
			_state &= ~eFiltered;
		}
	}

	const Matrix4& localToWorld() const;

	void transformChangedLocal();

	void transformChanged();

	void setTransformChangedCallback(const Callback& callback);

	// greebo: This gets called as soon as a scene::Node gets inserted into
	// the TraversableNodeSet. This triggers an instantiation call on the child node.
	virtual void onChildAdded(const INodePtr& child);
	virtual void onChildRemoved(const INodePtr& child);

	// Gets called when this node is inserted into a scene graph
	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

	// Returns TRUE if this node is inserted in the scene, FALSE otherwise
	bool inScene() const
	{
		return _instantiated;
	}

	/**
	 * greebo: Constructs the scene path to this node. This will walk up the
	 * ancestors until it reaches the top node, so don't expect this to be
	 * super fast.
	 */
	scene::Path getPath();

	// Returns a shared reference to this node
	scene::INodePtr getSelf();

	const IRenderEntityPtr& getRenderEntity() const
	{
		return _renderEntity;
	}

	// Set the render entity this node is attached to
	void setRenderEntity(const IRenderEntityPtr& entity)
	{
		_renderEntity = entity;
	}

	// Base renderable implementation
	virtual RenderSystemPtr getRenderSystem() const;
	virtual void setRenderSystem(const RenderSystemPtr& renderSystem);

protected:
	// Fills in the ancestors and self (in this order) into the given targetPath.
	void getPathRecursively(scene::Path& targetPath);

	TraversableNodeSet& getTraversable();

	virtual void instanceAttach(MapFile* mapfile);
	virtual void instanceDetach(MapFile* mapfile);

	// Clears the TraversableNodeSet
	virtual void removeAllChildNodes();

private:
	void evaluateBounds() const;
	void evaluateChildBounds() const;
	void evaluateTransform() const;
};

typedef boost::shared_ptr<Node> NodePtr;

} // namespace scene
