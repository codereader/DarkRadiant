#pragma once

#include "inode.h"
#include "ipath.h"
#include "irender.h"
#include <list>
#include "TraversableNodeSet.h"
#include "math/AABB.h"
#include "math/Matrix4.h"
#include "generic/callback.h"

class IUndoSystem;

namespace scene
{

class Graph;
typedef std::weak_ptr<Graph> GraphWeakPtr;

/// Main implementation of INode
class Node : public virtual INode, public std::enable_shared_from_this<Node>
{
public:
	enum {
		eVisible = 0,
		eHidden = 1 << 0,    // manually hidden by the user
		eFiltered = 1 << 1,  // excluded due to filter settings
		eExcluded = 1 << 2,  // excluded due to regioning or merging
		eLayered = 1 << 3,    // invisible at the current layer settings
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

	mutable Matrix4 _local2world;

	// Is true when the node is part of the scenegraph
	bool _instantiated;

	// A special flag capable of overriding the ordinary state flags
	// We use this to force the rendering of hidden but selected nodes
	bool _forceVisible;

	// The list of layers this object is associated to
	LayerList _layers;

protected:
	// If this node is attached to a parent entity, this is the reference to it
    IRenderEntity* _renderEntity;

	// The render system for passing down the child hierarchy later on
	RenderSystemWeakPtr _renderSystem;

	// The scene::Graph we're belonging to (weak reference to avoid circular references)
	GraphWeakPtr _sceneGraph;

public:
	Node();
	Node(const Node& other);

    virtual ~Node() {}

	static void resetIds();
	static unsigned long getNewId();

    // Default name for generic nodes
    std::string name() const override { return "node"; }

	void setSceneGraph(const GraphPtr& sceneGraph) override;

	bool isRoot() const override;
	void setIsRoot(bool isRoot) override;
	IMapRootNodePtr getRootNode() override;

	void enable(unsigned int state) override;
	void disable(unsigned int state) override;
    bool checkStateFlag(unsigned int state) const override;
    virtual bool supportsStateFlag(unsigned int state) const override;

	bool visible() const override;
	bool excluded() const override;

	// Layered implementation
	virtual void addToLayer(int layerId) override;
    virtual void removeFromLayer(int layerId) override;
	virtual void moveToLayer(int layerId) override;
    virtual const LayerList& getLayers() const override;
	virtual void assignToLayers(const LayerList& newLayers) override;

	virtual void addChildNode(const INodePtr& node) override;
	virtual void addChildNodeToFront(const INodePtr& node) override;
	virtual void removeChildNode(const INodePtr& node) override;
	virtual bool hasChildNodes() const override;

	virtual void traverse(NodeVisitor& visitor) override;
	virtual void traverseChildren(NodeVisitor& visitor) const override;
	virtual bool foreachNode(const VisitorFunc& functor) const override;

	virtual void setParent(const INodePtr& parent) override;
	virtual scene::INodePtr getParent() const override;

	const AABB& worldAABB() const override;

	const AABB& childBounds() const;

	virtual void boundsChanged() override;

	/**
	 * Return the filtered status of this Instance.
	 */
	virtual bool isFiltered() const override
    {
		return (_state & eFiltered) != 0;
	}

	/**
	 * Set the filtered status of this Node. Setting filtered to true will
	 * prevent the node from being rendered.
	 */
	virtual void setFiltered(bool filtered) override
    {
		if (filtered)
        {
			enable(eFiltered);
		}
		else
        {
			disable(eFiltered);
		}
	}

	const Matrix4& localToWorld() const override;

	virtual void transformChangedLocal() override;

	void transformChanged() override;

	// greebo: This gets called as soon as a scene::Node gets inserted into
	// the TraversableNodeSet. This triggers an instantiation call on the child node.
	virtual void onChildAdded(const INodePtr& child);
	virtual void onChildRemoved(const INodePtr& child);

	// Gets called when this node is inserted into a scene graph
	virtual void onInsertIntoScene(IMapRootNode& root) override;
	virtual void onRemoveFromScene(IMapRootNode& root) override;

	// Returns TRUE if this node is inserted in the scene, FALSE otherwise
	bool inScene() const override
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
	scene::INodePtr getSelf() override;

	IRenderEntity* getRenderEntity() const override
	{
		return _renderEntity;
	}

	// Set the render entity this node is attached to
	void setRenderEntity(IRenderEntity* entity) override
	{
		_renderEntity = entity;
	}

	// Base renderable implementation
	virtual RenderSystemPtr getRenderSystem() const;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

protected:
    // Set the "forced visible" flag, only to be used internally by subclasses
	virtual void setForcedVisibility(bool forceVisible, bool includeChildren) override;

	// Method for subclasses to check whether this node is forcedly visible
	bool isForcedVisible() const;

    // Overridable method to get notified on visibility changes of this node
    virtual void onVisibilityChanged(bool isVisibleNow)
    {}

	// Fills in the ancestors and self (in this order) into the given targetPath.
	void getPathRecursively(scene::Path& targetPath);

	TraversableNodeSet& getTraversable();

	// Clears the TraversableNodeSet
	virtual void removeAllChildNodes();

private:
    void connectUndoSystem(IUndoSystem& undoSystem);
    void disconnectUndoSystem(IUndoSystem& undoSystem);

	void evaluateBounds() const;
	void evaluateChildBounds() const;
	void evaluateTransform() const;
};

typedef std::shared_ptr<Node> NodePtr;

} // namespace scene
