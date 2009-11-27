#ifndef _OCTREE_NODE_H_
#define _OCTREE_NODE_H_

#include "inode.h"
#include "ispacepartition.h"
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace scene
{
	// The number of members, before the node tries to subdivide itself
	const std::size_t SUBDIVISION_THRESHOLD = 32;
	const std::size_t MIN_NODE_SIZE = 128;

class OctreeNode;
typedef boost::shared_ptr<OctreeNode> OctreeNodePtr;

class OctreeNode : 
	public ISPNode,
	public boost::enable_shared_from_this<OctreeNode>
{
protected:
	// The owning octree
	Octree& _owner;

	AABB _bounds;

	ISPNodeWeakPtr _parent;

	// The child nodes (8 or 0)
	NodeList _children;

	// The scene::INodePtrs contained in this octree node
	MemberList _members;

public:
	// Default constructor (invalid bounds)
	OctreeNode(Octree& owner, const OctreeNodePtr& parent = OctreeNodePtr()) :
		_owner(owner),
		_parent(parent)
	{
		_members.reserve(SUBDIVISION_THRESHOLD);
	}

	// Construct a node using bounds
	OctreeNode(Octree& owner, const AABB& bounds, const OctreeNodePtr& parent = OctreeNodePtr()) :
		_owner(owner),
		_bounds(bounds),
		_parent(parent)
	{
		_members.reserve(SUBDIVISION_THRESHOLD);
	}

	// Construct a node using AABB components
	OctreeNode(Octree& owner, const Vector3& origin, const Vector3& extents, 
			   const OctreeNodePtr& parent = OctreeNodePtr()) :
		_owner(owner),
		_bounds(origin, extents),
		_parent(parent)
	{
		_members.reserve(SUBDIVISION_THRESHOLD);
	}

	// Get the parent node (can be NULL for the root node)
	ISPNodePtr getParent() const 
	{
		return _parent.lock();
	}

	// The maximum bounds of this node
	const AABB& getBounds() const
	{
		return _bounds;
	}
	
	// The child nodes of this node (either 8 or 0)
	const NodeList& getChildNodes() const 
	{
		return _children;
	}

	// Get a list of members
	const MemberList& getMembers() const 
	{
		return _members;
	}

	// Returns true if no more child nodes are below this one
	bool isLeaf() const 
	{
		return _children.empty();
	}

	// Subdivide this octree node (adding 8 child nodes)
	void subdivide()
	{
		// Allocate 8 nodes
		_children.resize(8);

		Vector3 childExtents = _bounds.extents * 0.5;

		// Construct delta-vectors, pointing in each room direction
		Vector3 x(childExtents.x(), 0, 0);
		Vector3 y(0, childExtents.y(), 0);
		Vector3 z(0, 0, childExtents.z());


		Vector3 baseUpper = _bounds.origin + z;
		Vector3 baseLower = _bounds.origin - z;

		// Upper half of the cube
		_children[0] = OctreeNodePtr(new OctreeNode(_owner, baseUpper + x + y, childExtents, shared_from_this()));
		_children[1] = OctreeNodePtr(new OctreeNode(_owner, baseUpper + x - y, childExtents, shared_from_this()));
		_children[2] = OctreeNodePtr(new OctreeNode(_owner, baseUpper - x - y, childExtents, shared_from_this()));
		_children[3] = OctreeNodePtr(new OctreeNode(_owner, baseUpper - x + y, childExtents, shared_from_this()));

		// Lower half of the cube
		_children[4] = OctreeNodePtr(new OctreeNode(_owner, baseLower + x + y, childExtents, shared_from_this()));
		_children[5] = OctreeNodePtr(new OctreeNode(_owner, baseLower + x - y, childExtents, shared_from_this()));
		_children[6] = OctreeNodePtr(new OctreeNode(_owner, baseLower - x - y, childExtents, shared_from_this()));
		_children[7] = OctreeNodePtr(new OctreeNode(_owner, baseLower - x + y, childExtents, shared_from_this()));
	}

	// Indexing operator to retrieve a certain child
	OctreeNode& operator[](std::size_t index)
	{
		assert(index <= 7);
		assert(!_children.empty());

		return static_cast<OctreeNode&>(*_children[index]);
	}

	// This method moves all the contents (members and children) of this node to the "other" target node
	// If this node has children, the target node must be a leaf (this method won't override existing children)
	// Members will be moved to the end of the target's memberlist.
	void relocateContentsTo(OctreeNode& target)
	{
		assert(isLeaf() || target.isLeaf());

		// Copy all members from here to the target
		target._members.insert(target._members.end(), _members.begin(), _members.end());

		// Notify the Octree about the relocation
		for (ISPNode::MemberList::iterator i = _members.begin(); i != _members.end(); ++i)
		{
			_owner.notifyUnlink(*i, this);
			_owner.notifyLink(*i, &target);
		}

		// Clear our own member list
		_members.clear();

		// Just overwrite the target's child list, the assertion above makes sure nothing is overwritten
		target._children.swap(_children);
		_children.clear();
	}

	void addMember(const scene::INodePtr& sceneNode)
	{
		// Add to the internal list
		_members.push_back(sceneNode);

		// Notify the Octree to update lookup caches
		_owner.notifyLink(sceneNode, this);
	}

	// Links the given scene object into the tree
	OctreeNode* linkRecursively(const scene::INodePtr& sceneNode)
	{
		const AABB& bounds = sceneNode->worldAABB();

		// If the AABB is not valid, just link it here
		if (!bounds.isValid()) 
		{
			addMember(sceneNode);
			return this;
		}

		// This node has children, check if this object fits into one of our children
		for (std::size_t i = 0, size = _children.size(); i < size; ++i)
		{
			OctreeNode& child = static_cast<OctreeNode&>(*_children[i]);

			if (child.getBounds().contains(bounds))
			{
				// Node fits exactly into one of the children, enter recursion
				return child.linkRecursively(sceneNode);
			}
		}

		// Node didn't fit into any of the children, link it here
		addMember(sceneNode);

		// If this is a leaf, check if we exceeded the subdivision threshold
		if (isLeaf() && _members.size() >= SUBDIVISION_THRESHOLD && _bounds.extents.x() > MIN_NODE_SIZE)
		{
			// This leaf has enough members to justify a further subdivision, create 8 child nodes
			subdivide();

			// We cannot use the original _members vector in the loop below (iterator invalidation)...
			ISPNode::MemberList oldList;

			// ... so move the member list here
			oldList.swap(_members);

			// Cycle through all the members and distribute them over the children
			for (ISPNode::MemberList::iterator i = oldList.begin(); i != oldList.end(); ++i)
			{
				// Notify the owner about the re-link
				_owner.notifyUnlink(*i, this);

				// Call ourselves. The fact that we have 8 children now ensures that we won't be 
				// going down the same code path here again
				linkRecursively(*i);
			}
		}

		return this;
	}

	void unlink(const scene::INodePtr& sceneNode)
	{
		for (ISPNode::MemberList::iterator i = _members.begin(); i != _members.end(); ++i)
		{
			if (*i == sceneNode)
			{
				// Remove from the members list
				_members.erase(i);

				// Let the Octree know about this
				_owner.notifyUnlink(sceneNode, this);

				break;
			}
		}
	}

	void render(const RenderInfo& info) const
	{
		float numItems = _members.size() > 2 ? 1 : (_members.size() > 0 ? 0.6 : 0);
		glColor3f(numItems, numItems, numItems);

		AABB rb(_bounds);

		// Extend the renderbounds *slightly* so that the lines don't overlap
		rb.extents *= 1.02f;

		// Wireframe cuboid
		glBegin(GL_LINES);
			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());

			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());
			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());

			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());

			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());
			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());
			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());

			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + rb.extents.z());
			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() +  rb.extents.z());
			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());

			glVertex3f(rb.origin.x() + rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());
			glVertex3f(rb.origin.x() + -rb.extents.x(), rb.origin.y() + -rb.extents.y(), rb.origin.z() + -rb.extents.z());
		glEnd();

		for (std::size_t i = 0; i < _children.size(); ++i)
		{
			static_cast<OctreeNode&>(*_children[i]).render(info);
		}
	}
};

} // namespace scene

#endif /* _OCTREE_NODE_H_ */
