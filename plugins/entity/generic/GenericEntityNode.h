#ifndef GENERICENTITYNODE_H_
#define GENERICENTITYNODE_H_

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

namespace entity {

class GenericEntityNode :
	public EntityNode,
	public Snappable,
	public SelectionTestable,
	public Bounded
{
	friend class GenericEntity;

	GenericEntity m_contained;

public:
	GenericEntityNode(const IEntityClassPtr& eclass);
	GenericEntityNode(const GenericEntityNode& other);

	void construct();

	// Snappable implementation
	virtual void snapto(float snap);

	// EntityNode implementation
	virtual void refreshModel();

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
};
typedef boost::shared_ptr<GenericEntityNode> GenericEntityNodePtr;

} // namespace entity

#endif /*GENERICENTITYNODE_H_*/
