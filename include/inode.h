#pragma once

#include "Bounded.h"
#include "ilayer.h"
#include "irenderable.h"

#include <set>
#include <string>
#include <memory>

class AABB;
class Matrix4;

class IRenderEntity;
typedef std::shared_ptr<IRenderEntity> IRenderEntityPtr;

namespace scene
{

/**
* Interface for objects which can be filtered by the FilterSystem.
*/
class Filterable
{
public:
    /**
	 * Destructor
	 */
	virtual ~Filterable() {}

	/**
	 * Return the filtered state of this object. Returns true if the object is
	 * hidden due to filtering, and false if it is visible.
	 *
	 * It is up to the object's implementation to determine what criteria are
	 * used for filtering (texture, entityclass etc), however these criteria
	 * MUST be controlled by the filter system, and cannot include other
	 * arbitrary criteria such as the time of day or available memory.
	 *
	 * Note that this is not the primary function to discard objects during
	 * rendering or selection test. Use INode::visible() instead, which includes
	 * the current "filtered" setting.
	 */
	virtual bool isFiltered() const = 0;

	// Set the filtered status of this object
	virtual void setFiltered(bool filtered) = 0;
};

class INode;
typedef std::shared_ptr<INode> INodePtr;
typedef std::weak_ptr<INode> INodeWeakPtr;

class IMapRootNode;
typedef std::shared_ptr<IMapRootNode> IMapRootNodePtr;

class Graph;
typedef std::shared_ptr<Graph> GraphPtr;

class NodeVisitor
{
public:
    /**
	 * Destructor
	 */
	virtual ~NodeVisitor() {}

	/**
	 * greebo: Gets called before the children are traversed.
	 * Return TRUE to traverse the children, FALSE to prevent this.
	 */
	virtual bool pre(const INodePtr& node) = 0;

	/**
	 * greebo: Optional post-traverse call, gets invoked after the children
	 *         of this node have been traversed.
	 */
	virtual void post(const INodePtr& node) {}
};

/**
 * \brief Main interface for a Node, a basic element of the scenegraph.
 *
 * All nodes share a certain set of functionality, such as being placed in
 * layers, being able to render themselves, and being able to hold and
 * transform a list of child nodes.
 */
class INode :
	public Layered,
	public Filterable,
	public Bounded,
	public Renderable
{
public:
	enum class Type
	{
		Unknown = 0,
		MapRoot,
		Entity,
		Brush,
        Patch,
		Model,
		Particle,
        EntityConnection,
        MergeAction,
	};

public:

	virtual ~INode() {}

    /// Get the user-friendly string name of this node.
    virtual std::string name() const = 0;

	// Returns the type of this node
	virtual Type getNodeType() const = 0;

	/**
	 * Set the scenegraph this node is belonging to. This is usually
	 * set by the scenegraph itself during insertion.
	 */
	virtual void setSceneGraph(const GraphPtr& sceneGraph) = 0;

	/** greebo: Returns true, if the node is the root element
	 * 			of the scenegraph.
	 */
	virtual bool isRoot() const = 0;

	/** greebo: Sets the "isRoot" flag of this node.
	 */
	virtual void setIsRoot(bool isRoot) = 0;

	//  Searching this node's ancestry, this returns the toplevel/root node
	virtual IMapRootNodePtr getRootNode() = 0;

	/** greebo: State bit accessor methods. This enables/disables
	 * 			the bit of the state flag (e.g. hidden, excluded)
	 */
	virtual void enable(unsigned int state) = 0;
	virtual void disable(unsigned int state) = 0;

    // Returns true if the given state bit mask is set, false otherwise
    virtual bool checkStateFlag(unsigned int state) const = 0;

    // Returns true if this node supports the given state flag
    virtual bool supportsStateFlag(unsigned int state) const = 0;

	/** greebo: Returns true, if the node is not hidden by
	 * 			exclusion, filtering or anything else.
	 */
	virtual bool visible() const = 0;

	/** greebo: Returns true, if the node is excluded (eExcluded flag set)
	 */
	virtual bool excluded() const = 0;

	// Set the "forced visible" flag, which overrides the ordinary filtered/excluded state
	// This is used to force the rendering of nodes that are selected but would otherwise
	// be hidden due to the filtered/layered state.
	virtual void setForcedVisibility(bool forceVisible, bool includeChildren) = 0;

	// Child node handling

	// Adds a new child node (is appended at the end of the list of existing children)
	virtual void addChildNode(const INodePtr& node) = 0;

	// Adds a new child node (is inserted at the front of any existing children)
	// Useful in special scenarios like when adding a world spawn node.
	virtual void addChildNodeToFront(const INodePtr& node) = 0;

	virtual void removeChildNode(const INodePtr& node) = 0;
	virtual bool hasChildNodes() const = 0;

	/**
	 * greebo: Traverses this node and all child nodes (recursively)
	 * using the given visitor.
	 *
	 * Note: replaces the legacy Node_traverseSubgraph() method.
	 */
	virtual void traverse(NodeVisitor& visitor) = 0;

	/**
	 * greebo: Traverses all child nodes (recursively) using the given visitor.
	 * Note: this will NOT visit the current node.
	 */
	virtual void traverseChildren(NodeVisitor& visitor) const = 0;

	/**
	 * Traversal function which can be used to hit all nodes in a
	 * graph or collection. If the functor returns false traversal stops.
	 */
	typedef std::function<bool(const scene::INodePtr&)> VisitorFunc;

	/**
	 * Call the given functor for each child node, depth first
	 * This is a simpler alternative to the usual traverse() method
	 * which provides pre() and post() methods and more control about
	 * which nodes to traverse and. This forEachNode() routine simply
	 * hits every child node including their children.
	 *
	 * @returns: true if the functor returned false on any of the
	 * visited nodes. The return type is used to pass the stop signal
	 * up the stack during traversal.
	 */
	virtual bool foreachNode(const VisitorFunc& functor) const = 0;

	/**
	 * Returns a shared_ptr to itself.
	 */
	virtual scene::INodePtr getSelf() = 0;

	// Set the parent of this node, is called on insertion in a traversable
	virtual void setParent(const INodePtr& parent) = 0;
	virtual scene::INodePtr getParent() const = 0;

	/**
	 * greebo: Gets called after the node has been inserted into the scene.
	 */
	virtual void onInsertIntoScene(IMapRootNode& root) = 0;

	/**
	 * greebo: This gets called by the SceneGraph before the Node is actually
	 * removed from the scene. This gives the node the opportunity to
	 * change its "selected" status or anything else.
	 */
    virtual void onRemoveFromScene(IMapRootNode& root) = 0;

	/**
	 * Returns true if this node is in the scene
	 */
	virtual bool inScene() const = 0;

	// Get/Set the render entity this node is attached to
	virtual IRenderEntity* getRenderEntity() const = 0;
	virtual void setRenderEntity(IRenderEntity* entity) = 0;

	// Call this if the node gets changed in any way or gets inserted somewhere.
	virtual void boundsChanged() = 0;
	// Call this on transform change
	virtual void transformChanged() = 0;

	// Returns the bounds in world coordinates
	virtual const AABB& worldAABB() const = 0;

	/**
     * \brief Return the transformation from local to world coordinates
     *
     * This represents the final transformation from this node's own coordinate
     * space into world space, including any transformations inherited from
     * parent nodes.
     */
	virtual const Matrix4& localToWorld() const = 0;

	// Undo/Redo events - some nodes need to do extra legwork after undo or redo
	// This is called by the TraversableNodeSet after a undo/redo operation
	// not by the UndoSystem itself, at least not yet.
	virtual void onPostUndo() {}
	virtual void onPostRedo() {}

    // Called during recursive transform changed, but only by INodes themselves
    virtual void transformChangedLocal() = 0;
};

/// Cast an INode to a particular interface
template<typename Interface>
std::shared_ptr<Interface> node_cast(INodePtr nodeP)
{
    return std::dynamic_pointer_cast<Interface>(nodeP);
}

} // namespace scene
