#pragma once

#include "nameable.h"
#include "editable.h"
#include "inamespace.h"

#include "scenelib.h"
#include "selectionlib.h"
#include "transformlib.h"
#include "irenderable.h"

#include "GenericEntity.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"

namespace entity
{

class GenericEntityNode;
typedef boost::shared_ptr<GenericEntityNode> GenericEntityNodePtr;

class GenericEntityNode :
	public EntityNode,
	public Snappable
{
	friend class GenericEntity;

	GenericEntity m_contained;

public:
	GenericEntityNode(const IEntityClassPtr& eclass);

private:
	GenericEntityNode(const GenericEntityNode& other);

public:
	static GenericEntityNodePtr Create(const IEntityClassPtr& eclass);

	// Snappable implementation
	virtual void snapto(float snap);

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test);

	scene::INodePtr clone() const;

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
	void _onTransformationChanged();

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
	void _applyTransformation();

	// Override EntityNode::construct()
	void construct();
};

} // namespace entity
