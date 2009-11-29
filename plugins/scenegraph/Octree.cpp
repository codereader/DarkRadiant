#include "Octree.h"

#include "inode.h"
#include "iregistry.h"

#include "OctreeNode.h"

namespace scene
{

namespace
{
	const double START_SIZE = 512.0;

	const AABB START_AABB(Vector3(0,0,0), Vector3(START_SIZE, START_SIZE, START_SIZE));
}

Octree::Octree() :
	_root(new OctreeNode(*this, START_AABB)),
	_shader(GlobalRenderSystem().capture("[1 0 0]"))
{
	GlobalRenderSystem().attachRenderable(*this);
}

Octree::~Octree()
{
	GlobalRenderSystem().detachRenderable(*this);

	_nodeMapping.clear();
	_root = OctreeNodePtr();
}

void Octree::link(const scene::INodePtr& sceneNode)
{
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
		if (newBounds.extents.x() > GlobalRegistry().getFloat("game/defaults/maxWorldCoord"))
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
bool Octree::unLink(const scene::INodePtr& sceneNode)
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

void Octree::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const 
{
	collector.SetState(_shader, RenderableCollector::eFullMaterials);

	collector.addRenderable(*this, Matrix4::getIdentity());
}

void Octree::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const 
{
	collector.SetState(_shader, RenderableCollector::eWireframeOnly);

	collector.addRenderable(*this, Matrix4::getIdentity());
}

void Octree::render(const RenderInfo& info) const
{
	_root->render(info);
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

void Octree::notifyErase(OctreeNode* node)
{
	// Remove the node from the lookup table, if found
	for (NodeMapping::iterator i = _nodeMapping.begin(); i != _nodeMapping.end(); ++i)
	{
		if (i->second == node)
		{
			int i = 6;
		}
	}
}

} // namespace scene
