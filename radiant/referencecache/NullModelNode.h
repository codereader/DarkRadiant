#pragma once

#include "scenelib.h"
#include "Bounded.h"
#include "irenderable.h"

#include "NullModel.h"

namespace model {

class NullModelNode;
typedef boost::shared_ptr<NullModelNode> NullModelNodePtr;

class NullModelNode :
	public scene::Node,
	public SelectionTestable,
	public ModelNode
{
private:
	NullModelPtr _nullModel;
public:
	// Default constructor, allocates a new NullModel
	NullModelNode();

	// Alternative constructor, uses the given nullModel
	NullModelNode(const NullModelPtr& nullModel);

	// Accessor to the singleton instance
	static NullModelNodePtr InstancePtr();

	virtual const IModel& getIModel() const;

	void testSelect(Selector& selector, SelectionTest& test);

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	bool isHighlighted() const
	{
		return false; // never highlighted
	}

	// Bounded implementation
	virtual const AABB& localAABB() const;
};

} // namespace model
