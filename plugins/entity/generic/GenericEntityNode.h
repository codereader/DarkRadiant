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
typedef boost::shared_ptr<GenericEntityNode> GenericEntityNodePtr;

class GenericEntityNode :
	public EntityNode,
	public Snappable,
	public Editable
{
	friend class GenericEntity;

	GenericEntity m_contained;

	// The local pivot of this generic node is always at the local origin 0,0,0
	Matrix4 _localPivot;

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

	// Override EntityNode::getDirection()
	const Vector3& getDirection() const;

	// Editable - to prevent the selection system from including particle bounds in the pivot calculation
	const Matrix4& getLocalPivot() const;

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
