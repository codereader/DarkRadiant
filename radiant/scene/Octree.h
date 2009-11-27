#ifndef _OCTREE_H_
#define _OCTREE_H_

#include "ispacepartition.h"
#include "irender.h"
#include "irenderable.h"
#include <map>

#include "OctreeNode.h"

namespace scene 
{

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
	ISPNodePtr link(const scene::INodePtr& sceneNode);

	// Unlink this node from the SP tree
	void unLink(const scene::INodePtr& sceneNode);

	// Returns the root node of this SP tree
	ISPNodePtr getRoot() const;

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	void render(const RenderInfo& info) const;
};

} // namespace scene

#endif /* _OCTREE_H_ */
