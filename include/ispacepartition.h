#ifndef _ISPACE_PARTITION_H_
#define _ISPACE_PARTITION_H_

#include <list>
#include <vector>
#include "imodule.h"

// Forward declaration
class AABB;

namespace scene
{

// Some forward declarations to avoid including all the headers
class INode;
typedef boost::shared_ptr<INode> INodePtr;

class ISPNode;
typedef boost::shared_ptr<ISPNode> ISPNodePtr;
typedef boost::weak_ptr<ISPNode> ISPNodeWeakPtr;

/**
 * greebo: This is the abstract definition of a SpacePartition node.
 *
 * A SpacePartition node has the following properties:
 *
 * - It always has valid bounds (in the form of an AABB).
 * - Each node can have any amount of children [0..infinity)
 * - A node with 0 children is called a Leaf.
 * - Each node can have exactly one parent (which is NULL for the root node).
 * - The collectivity of nodes form a tree whereas the topmost one is the largest.
 * - Each node can host any amount of "members" (member == scene::INode).
 *
 * It is the task of the ISpacePartitionSystem to allocate and manage these nodes.
 * scene::INodes are "linked" to the correct ISPNodes through the ISPacePartition's
 * link() methods - and are unlinked through the unlink() method of the latter.
 */
class ISPNode
{
public:
	virtual ~ISPNode() {}

	// The child nodes
	typedef std::vector<ISPNodePtr> NodeList;

	// The members
	typedef std::list<INodePtr> MemberList;

	// Get the parent node (can be NULL for the root node)
	virtual ISPNodePtr getParent() const = 0;

	// The maximum bounds of this node
	virtual const AABB& getBounds() const = 0;

	// The child nodes of this node (either 8 or 0)
	virtual const NodeList& getChildNodes() const = 0;

	// Returns true if no more child nodes are below this one
	virtual bool isLeaf() const = 0;

	// Get a list of members
	virtual const MemberList& getMembers() const = 0;
};
typedef boost::shared_ptr<ISPNode> ISPNodePtr;

/**
 * greebo: The SpacePartitionSystem interface is a simple one. All it needs
 * to do is to provide link/unlink methods for linking scene::INodes
 * into the space partition system and to deliver the "entry point" for traversal,
 * which is the root ISPNode.
 *
 * The link() method makes sure the given node is added as member to the ISPNode it fits best.
 * The unlink() method can be used to remove a node from the tree again.
 *
 * Note: It's not allowed to call link() for nodes which are already linked into the tree.
 * It's safe to call unlink() for any node at any time, even multiple times in a row.
 * The unlink() method will return true if the node had been linked before.
 */
class ISpacePartitionSystem
{
public:
	virtual ~ISpacePartitionSystem() {}

	// Links this node into the SP tree. Returns the node it ends up being associated with
	virtual void link(const scene::INodePtr& sceneNode) = 0;

	// Unlink this node from the SP tree, returns true if this was successful
	// (node had been linked before)
	virtual bool unlink(const scene::INodePtr& sceneNode) = 0;

	// Returns the root node of this SP tree (the largest one, encompassing everything)
	virtual ISPNodePtr getRoot() const = 0;
};
typedef boost::shared_ptr<ISpacePartitionSystem> ISpacePartitionSystemPtr;

} // namespace scene

#endif /* _ISPACE_PARTITION_H_ */
