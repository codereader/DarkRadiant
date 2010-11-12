#ifndef _OCTREE_H_
#define _OCTREE_H_

#include "ispacepartition.h"
#include <map>

namespace scene
{

class OctreeNode;
typedef boost::shared_ptr<OctreeNode> OctreeNodePtr;

/**
 * greebo: An Octree is a simple way to subdivide the entire space
 * used by a collectivity of nodes in a scene. This is achieved by using cubic
 * axis aligned OctreeNodes, which form a hierarchical tree.
 *
 * The topmost OctreeNode (the root) is a cube large enough to encompass
 * all linked nodes in the tree.
 *
 * Starting from the root, each OctreeNode in the tree can be further
 * subdivided to have exactly 8 child nodes of equal size, hence the name.
 *
 * This subdivision takes place when a certain amount of members (scene::INodes)
 * is exceeded - i.e. the Octree will dynamically subdivide itself on the fly,
 * until a certain lower limit is reached (usually the smallest OctreeNode
 * is a cube with an edge length of 256 units).
 *
 * The root OctreeNode's extents cannot be any larger than 65535 units.
 *
 * Each scene::INode is linked to exactly one OctreeNode, namely the smallest possible
 * one. When the scene::INode's bounds cannot exactly be squeezed into exactly
 * one OctreeNode, the scene::INode remains in the one parent node able to do so.
 * In the "worst" case this is the root node itself.
 *
 * The Octree maintains a lookup table (NodeMapping) to implement a fast unlink()
 * algorithm. The scene::INodes don't know or care where they are linked to, so
 * it needs a fast lookup to avoid having to traverse the entire tree to find and
 * remove a single node.
 */
class Octree :
	public ISpacePartitionSystem
{
private:
	// The root node of this SP
	OctreeNodePtr _root;

	// Maps scene nodes against octree nodes, for fast lookup during unlink
	typedef std::map<INodePtr, OctreeNode*> NodeMapping;
	NodeMapping _nodeMapping;

public:
	Octree();

	~Octree();

	// Links this node into the SP tree.
	void link(const scene::INodePtr& sceneNode);

	// Unlink this node from the SP tree, returns true if found
	bool unlink(const scene::INodePtr& sceneNode);

	// Returns the root node of this SP tree
	ISPNodePtr getRoot() const;

	// Callback used by the OctreeNodes to let the tree update its caching structures
	void notifyLink(const scene::INodePtr& sceneNode, OctreeNode* node);
	void notifyUnlink(const scene::INodePtr& sceneNode, OctreeNode* node);

#ifdef _DEBUG
	// In debug builds, this ensures that no octree node is deleted
	// while it is still mapped in the NodeMapping table
	void notifyErase(OctreeNode* node);
#endif

private:
	/**
	 * This is called whenever a node is linked into the octree
	 * and ensures that the topmost octree node (the root node) is
	 * large enough to encompass the scenenode's bounds.
	 */
	void ensureRootSize(const scene::INodePtr& sceneNode);
};

} // namespace scene

#endif /* _OCTREE_H_ */
