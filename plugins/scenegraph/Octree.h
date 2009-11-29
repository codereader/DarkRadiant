#ifndef _OCTREE_H_
#define _OCTREE_H_

#include "ispacepartition.h"
#include "irender.h"
#include "irenderable.h"
#include <map>

namespace scene 
{

class OctreeNode;
typedef boost::shared_ptr<OctreeNode> OctreeNodePtr;

class Octree :
	public ISpacePartitionSystem,
	public Renderable,
	public OpenGLRenderable
{
private:
	// The root node of this SP
	OctreeNodePtr _root;

	ShaderPtr _shader;

	// Maps scene nodes against octree nodes, for fast lookup during unlink
	typedef std::map<INodePtr, OctreeNode*> NodeMapping;
	NodeMapping _nodeMapping;

public:
	Octree();

	~Octree();

	// Links this node into the SP tree. Returns the node it ends up being associated with
	void link(const scene::INodePtr& sceneNode);

	// Unlink this node from the SP tree
	bool unLink(const scene::INodePtr& sceneNode);

	// Returns the root node of this SP tree
	ISPNodePtr getRoot() const;

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	void render(const RenderInfo& info) const;

	// Callback used by the OctreeNodes to let the tree update its caching structures
	void notifyLink(const scene::INodePtr& sceneNode, OctreeNode* node);
	void notifyUnlink(const scene::INodePtr& sceneNode, OctreeNode* node);

#ifdef _DEBUG
	void notifyErase(OctreeNode* node);
#endif

private:
	void ensureRootSize(const scene::INodePtr& sceneNode);
};

} // namespace scene

#endif /* _OCTREE_H_ */
