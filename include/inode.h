#ifndef INODE_H_
#define INODE_H_

#include "ilayer.h"
#include "iinstantiable.h"

#include <set>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class AABB;
class Matrix4;

namespace scene {

/**
* Interface for objects which can be filtered by the FilterSystem.
*/
class Filterable
{
public:
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
typedef boost::shared_ptr<INode> INodePtr;
typedef boost::weak_ptr<INode> INodeWeakPtr;

class NodeVisitor 
{
public:
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

/** greebo: Abstract definition of a Node, a basic element
 * 			of the scenegraph.
 */
class INode :
	public Layered,
	public Filterable,
	public Instantiable
{
public:
	/** greebo: Returns true, if the node is the root element
	 * 			of the scenegraph.
	 */
	virtual bool isRoot() const = 0;
	
	/** greebo: Sets the "isRoot" flag of this node.  
	 */
	virtual void setIsRoot(bool isRoot) = 0;
	
	/** greebo: State bit accessor methods. This enables/disables
	 * 			the bit of the state flag (e.g. hidden, excluded)
	 */
	virtual void enable(unsigned int state) = 0;
	virtual void disable(unsigned int state) = 0;

	/** greebo: Returns true, if the node is not hidden by 
	 * 			exclusion, filtering or anything else.
	 */
	virtual bool visible() const = 0;
	
	/** greebo: Returns true, if the node is excluded (eExcluded flag set)
	 */
	virtual bool excluded() const = 0;

	// Child node handling
	virtual void addChildNode(const INodePtr& node) = 0;
	virtual void removeChildNode(const INodePtr& node) = 0;
	virtual bool hasChildNodes() const = 0;

	/**
	 * greebo: Traverses all child nodes using the given visitor.
	 */
	virtual void traverse(NodeVisitor& visitor) = 0;

	/**
	 * greebo: The INode class holds a boost::weak_ptr of itself.
	 * Use these methods to initialise this internal weak ptr by passing
	 * a shared_ptr to setSelf(). 
	 *
	 * The method getSelf() tries to lock the weak_ptr and returns the 
	 * resulting shared_ptr.
	 */
	virtual void setSelf(const INodePtr& self) = 0;
	virtual scene::INodePtr getSelf() const = 0;

	// Set the parent of this node, is called on insertion in a traversable
	virtual void setParent(const INodePtr& parent) = 0;
	virtual scene::INodePtr getParent() const = 0;

	/**
	 * greebo: Gets called after the node has been inserted into the scene.
	 */
	virtual void onInsertIntoScene() {} // empty default implementation

	/**
	 * greebo: This gets called by the SceneGraph before the Node is actually 
	 * removed from the scene. This gives the node the opportunity to 
	 * change its "selected" status or anything else.
	 */
	virtual void onRemoveFromScene() {} // empty default implementation

	// Call this if the node gets changed in any way or gets inserted somewhere.
	virtual void boundsChanged() = 0;
	// Call this on transform change
	virtual void transformChanged() = 0;

	// Returns the bounds in world coordinates
	virtual const AABB& worldAABB() const = 0;

	// Returns the transformation from local to world coordinates
	virtual const Matrix4& localToWorld() const = 0;
};

} // namespace scene

#endif /*INODE_H_*/
