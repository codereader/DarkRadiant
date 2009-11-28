#ifndef _ISPACE_PARTITION_H_
#define _ISPACE_PARTITION_H_

#include <vector>
#include "imodule.h"
#include "math/aabb.h"

namespace scene
{
class INode;
typedef boost::shared_ptr<INode> INodePtr;

class ISPNode;
typedef boost::shared_ptr<ISPNode> ISPNodePtr;
typedef boost::weak_ptr<ISPNode> ISPNodeWeakPtr;

class ISPNode
{
public:
	virtual ~ISPNode() {}

	// The child nodes
	typedef std::vector<ISPNodePtr> NodeList;

	// The members
	typedef std::vector<INodePtr> MemberList;

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

class ISpacePartitionSystem
{
public:
	virtual ~ISpacePartitionSystem() {}

	// Links this node into the SP tree. Returns the node it ends up being associated with
	virtual void link(const scene::INodePtr& sceneNode) = 0;

	// Unlink this node from the SP tree, returns true if found
	virtual bool unLink(const scene::INodePtr& sceneNode) = 0;

	// Returns the root node of this SP tree
	virtual ISPNodePtr getRoot() const = 0;
};
typedef boost::shared_ptr<ISpacePartitionSystem> ISpacePartitionSystemPtr;

class ISpacePartitionSystemFactory :
	public RegisterableModule
{
public:
	virtual ~ISpacePartitionSystemFactory() {}

	// Creates a new space partition system for use in a scene
	virtual ISpacePartitionSystemPtr create() = 0;
};

} // namespace scene

const std::string MODULE_SPACE_PARTITION_FACTORY("Space Partition System Factory");

inline scene::ISpacePartitionSystemFactory& GlobalSpacePartitionSystemFactory()
{
	// Cache the reference locally
	static scene::ISpacePartitionSystemFactory& _factory(
		*boost::static_pointer_cast<scene::ISpacePartitionSystemFactory>(
			module::GlobalModuleRegistry().getModule(MODULE_SPACE_PARTITION_FACTORY)
		)
	);
	return _factory;
}

#endif /* _ISPACE_PARTITION_H_ */
