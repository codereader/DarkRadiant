#ifndef _OCTREE_NODE_H_
#define _OCTREE_NODE_H_

#include "ispacepartition.h"
#include <boost/weak_ptr.hpp>

namespace scene
{

class OctreeNode;
typedef boost::shared_ptr<OctreeNode> OctreeNodePtr;

class OctreeNode : 
	public ISPNode
{
protected:
	AABB _bounds;

	ISPNodeWeakPtr _parent;

	// The child nodes (8 or 0)
	NodeList _children;

	// The scene::INodePtrs contained in this octree node
	MemberList _members;

public:
	// Default constructor (invalid bounds)
	OctreeNode()
	{}

	// Construct a node using bounds
	OctreeNode(const AABB& bounds) :
		_bounds(bounds)
	{}

	// Construct a node using AABB components
	OctreeNode(const Vector3& origin, const Vector3& extents) :
		_bounds(origin, extents)
	{}

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
		_children[0] = OctreeNodePtr(new OctreeNode(baseUpper + x + y, childExtents));
		_children[1] = OctreeNodePtr(new OctreeNode(baseUpper + x - y, childExtents));
		_children[2] = OctreeNodePtr(new OctreeNode(baseUpper - x - y, childExtents));
		_children[3] = OctreeNodePtr(new OctreeNode(baseUpper - x + y, childExtents));

		// Lower half of the cube
		_children[4] = OctreeNodePtr(new OctreeNode(baseLower + x + y, childExtents));
		_children[5] = OctreeNodePtr(new OctreeNode(baseLower + x - y, childExtents));
		_children[6] = OctreeNodePtr(new OctreeNode(baseLower - x - y, childExtents));
		_children[7] = OctreeNodePtr(new OctreeNode(baseLower - x + y, childExtents));
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

		target._members.insert(target._members.end(), _members.begin(), _members.end());
		_members.clear();

		// Just overwrite the target's child list, the assertion above makes sure nothing is overwritten
		target._children.swap(_children);
		_children.clear();
	}

	void render(const RenderInfo& info) const
	{
		// Wireframe cuboid
		glBegin(GL_LINES);
			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());
			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());

			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());
			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());

			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());
			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());

			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());
			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());

			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());
			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());

			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());
			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());

			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());
			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());

			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());
			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());

			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());
			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());

			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + _bounds.extents.z());
			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + _bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());

			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() +  _bounds.extents.z());
			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());

			glVertex3f(_bounds.origin.x() + _bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());
			glVertex3f(_bounds.origin.x() + -_bounds.extents.x(), _bounds.origin.y() + -_bounds.extents.y(), _bounds.origin.z() + -_bounds.extents.z());
		glEnd();

		for (std::size_t i = 0; i < _children.size(); ++i)
		{
			static_cast<OctreeNode&>(*_children[i]).render(info);
		}
	}
};

} // namespace scene

#endif /* _OCTREE_NODE_H_ */
