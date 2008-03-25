#ifndef _NULLMODELNODE_H_
#define _NULLMODELNODE_H_

#include "scenelib.h"
#include "Bounded.h"
#include "cullable.h"
#include "irenderable.h"

#include "NullModel.h"

namespace model {

class NullModelNode;
typedef boost::shared_ptr<NullModelNode> NullModelNodePtr;

class NullModelNode : 
	public scene::Node, 
	public SelectionTestable,
	public Renderable,
	public Bounded,
	public Cullable,
	public ModelNode
{
	NullModelPtr _nullModel;
public:
	// Default constructor, allocates a new NullModel
	NullModelNode();

	// Alternative constructor, uses the given nullModel
	NullModelNode(const NullModelPtr& nullModel);

	// Accessor to the singleton instance
	static NullModelNodePtr InstancePtr();

	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);

	virtual const IModel& getIModel() const;

	void testSelect(Selector& selector, SelectionTest& test);

	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(
		const VolumeTest& test, const Matrix4& localToWorld) const;
};

} // namespace model

#endif /* _NULLMODELNODE_H_ */
