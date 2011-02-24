#include "Octree.h"

#include "inode.h"

#include "OctreeNode.h"

namespace scene
{

namespace
{
	const float START_SIZE = 512.0f;
	const float MAX_WORLD_COORD = 65536;

	const AABB START_AABB(Vector3(0,0,0), Vector3(START_SIZE, START_SIZE, START_SIZE));
}

Octree::Octree()
{
	_root = OctreeNodePtr(new OctreeNode(*this, START_AABB));
}

Octree::~Octree()
{
	_nodeMapping.clear();
	_root = OctreeNodePtr();
}

void Octree::link(const scene::INodePtr& sceneNode)
{
	// Make sure we don't do double-links
	assert(_nodeMapping.find(sceneNode) == _nodeMapping.end());

	// Make sure the root node is large enough
	ensureRootSize(sceneNode);

	// Root node size is adjusted, let's link the node into the smallest encompassing octant
	_root->linkRecursively(sceneNode);
}

void Octree::ensureRootSize(const scene::INodePtr& sceneNode)
{
	// Check if sceneNode exceeds the root node's bounds
	const AABB& aabb = sceneNode->worldAABB();

	if (!aabb.isValid()) return; // skip this for invalid bounds

	while (!_root->getBounds().contains(aabb))
	{
		// The bounding box of this node exceed the root node's bounds, we need to extend the tree bounds
		AABB newBounds = _root->getBounds();
		newBounds.extents *= 2;

		// Don't go beyond the map limits
		if (newBounds.extents.x() > MAX_WORLD_COORD)
		{
			break;
		}

		// Allocate a new root node and subdivide it once
		OctreeNodePtr newRootPtr(new OctreeNode(*this, newBounds));

		OctreeNode& newRoot = *newRootPtr;
		OctreeNode& oldRoot = *_root;

		// Re-link the members of the old root node
		// Note: this might be inaccurate, as some members of the old root could be
		// re-linked to some children of the new root. But we don't want to call
		// link again, as this can lead to re-entering of the evaluateBounds() function
		// in scene::Node in some cases.
		oldRoot.relocateMembersTo(newRoot);

		// Now, subdivide the new root node, after we moved the members
		newRoot.subdivide();

		// Check if the old root had children
		if (!oldRoot.isLeaf())
		{
			// Move the children of the old root into the new root
			// Each octant of the old root will be added to one child of the new root
			for (std::size_t i = 0; i < 8; ++i)
			{
				// Subdivide each of the new children
				newRoot[i].subdivide();

				// Find out which of the new subdivisions is matching the children of the old root
				for (std::size_t j = 0; j < 8; ++j)
				{
					for (std::size_t old = 0; old < 8; ++old)
					{
						OctreeNode& newNode = newRoot[i][j];

						if (newNode.getBounds() == oldRoot[old].getBounds())
						{
							oldRoot[old].relocateMembersTo(newNode);
							oldRoot[old].relocateChildrenTo(newNode);
							break;
						}
					}
				}
			}
		}

		_root = newRootPtr;
	}
}

// Unlink this node from the SP tree
bool Octree::unlink(const scene::INodePtr& sceneNode)
{
	NodeMapping::iterator found = _nodeMapping.find(sceneNode);

	if (found != _nodeMapping.end())
	{
		// Lookup successful, unlink the node (will fire notifyUnlink())
		found->second->unlink(sceneNode);
		return true;
	}

	return false;
}

// Returns the root node of this SP tree
ISPNodePtr Octree::getRoot() const
{
	return _root;
}

void Octree::notifyLink(const scene::INodePtr& sceneNode, OctreeNode* node)
{
	std::pair<NodeMapping::iterator, bool> result =
		_nodeMapping.insert(NodeMapping::value_type(sceneNode, node));

	assert(result.second);
}

void Octree::notifyUnlink(const scene::INodePtr& sceneNode, OctreeNode* node)
{
	// Remove the node from the lookup table, if found
	NodeMapping::iterator found = _nodeMapping.find(sceneNode);

	assert(found != _nodeMapping.end());

	_nodeMapping.erase(found);
}

#ifdef _DEBUG
void Octree::notifyErase(OctreeNode* node)
{
	// Remove the node from the lookup table, if found
	for (NodeMapping::iterator i = _nodeMapping.begin(); i != _nodeMapping.end(); ++i)
	{
		assert(i->second != node);
	}
}
#endif

} // namespace scene
