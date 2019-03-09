#pragma once

#include "editable.h"
#include "inamespace.h"

#include "selectionlib.h"
#include "transformlib.h"
#include "irenderable.h"

#include "GenericEntity.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"

namespace entity
{

class GenericEntityNode;
typedef std::shared_ptr<GenericEntityNode> GenericEntityNodePtr;

class GenericEntityNode :
	public EntityNode,
	public Snappable
{
	friend class GenericEntity;

	GenericEntity m_contained;

    // Whether to draw a solid/shaded box in full material render mode or just the wireframe
    enum SolidAAABBRenderMode
    {
        SolidBoxes,
        WireFrameOnly,
    };

    SolidAAABBRenderMode _solidAABBRenderMode;

public:
	GenericEntityNode(const IEntityClassPtr& eclass);

private:
	GenericEntityNode(const GenericEntityNode& other);

public:
	static GenericEntityNodePtr Create(const IEntityClassPtr& eclass);

	// Snappable implementation
	virtual void snapto(float snap) override;

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test) override;

	scene::INodePtr clone() const override;

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;

    SolidAAABBRenderMode getSolidAABBRenderMode() const;

	// Override EntityNode::getDirection()
	const Vector3& getDirection() const override;

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;

    virtual void onChildAdded(const scene::INodePtr& child) override;
	virtual void onChildRemoved(const scene::INodePtr& child) override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
    void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

	// Override EntityNode::construct()
	void construct() override;
};

} // namespace entity
