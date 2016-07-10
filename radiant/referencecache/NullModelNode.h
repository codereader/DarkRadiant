#pragma once

#include "Bounded.h"
#include "irenderable.h"

#include "NullModel.h"

namespace model {

class NullModelNode;
typedef std::shared_ptr<NullModelNode> NullModelNodePtr;

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

	std::string name() const;
	Type getNodeType() const;

	// Accessor to the singleton instance
	static NullModelNodePtr InstancePtr();

	virtual const IModel& getIModel() const;
	virtual IModel& getIModel();

	void testSelect(Selector& selector, SelectionTest& test);

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	std::size_t getHighlightFlags() const override
	{
		return Highlight::None; // never highlighted
	}

	// Bounded implementation
	virtual const AABB& localAABB() const;
};

} // namespace model
