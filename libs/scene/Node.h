#ifndef SCENE_NODE_H_
#define SCENE_NODE_H_

#include "inode.h"
#include "ipath.h"
#include <list>
#include "TraversableNodeSet.h"
#include "math/aabb.h"
#include "generic/callback.h"

namespace scene {

class Node :
	public INode,
	public Traversable::Observer
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

	// A weak reference to "self"
	INodeWeakPtr _self;

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

public:
	Node();	
	Node(const Node& other);
	
	static void resetIds();	
	static unsigned long getNewId();
	
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

	virtual void addChildNode(const INodePtr& node);
	virtual void removeChildNode(const INodePtr& node);
	virtual bool hasChildNodes() const;
	
	virtual void traverse(NodeVisitor& visitor);

	virtual void setSelf(const INodePtr& self);
	virtual scene::INodePtr getSelf() const;

	virtual void setParent(const INodePtr& parent);
	virtual scene::INodePtr getParent() const;

	const AABB& worldAABB() const;

	const AABB& childBounds() const;

	void boundsChanged();
	typedef MemberCaller<Node, &Node::boundsChanged> BoundsChangedCaller;

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
	typedef MemberCaller<Node, &Node::transformChanged> TransformChangedCaller;

	void setTransformChangedCallback(const Callback& callback);

	// traverse observer
	// greebo: This gets called as soon as a scene::Node gets inserted into
	// the oberved Traversable. This triggers an instantiation call and ensures
	// that each inserted node is also instantiated.
	void onTraversableInsert(INodePtr child);
	void onTraversableErase(INodePtr child);

	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);

	/**
	 * greebo: Constructs the scene path to this node. This will walk up the
	 * ancestors until it reaches the top node, so don't expect this to be
	 * super fast.
	 */
	scene::Path getPath() const;

protected:
	// Fills in the ancestors and self (in this order) into the given targetPath.
	void getPathRecursively(scene::Path& targetPath);

	TraversableNodeSet& getTraversable();

	virtual void attachTraverseObserver(scene::Traversable::Observer* observer);
	virtual void detachTraverseObserver(scene::Traversable::Observer* observer);

	virtual void instanceAttach(MapFile* mapfile);
	virtual void instanceDetach(MapFile* mapfile);

	// Clears the TraversableNodeSet
	virtual void removeAllChildNodes();

private:
	void evaluateBounds() const;
	void evaluateChildBounds() const;
	void evaluateTransform() const;
};

} // namespace scene

#endif /* SCENE_NODE_H_ */
