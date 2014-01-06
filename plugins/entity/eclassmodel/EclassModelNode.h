#pragma once

#include "inamespace.h"
#include "modelskin.h"
#include "ientity.h"
#include "iselection.h"

#include "scene/TraversableNodeSet.h"
#include "transformlib.h"
#include "selectionlib.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"
#include "../KeyObserverDelegate.h"

#include "EclassModel.h"

namespace entity
{

class EclassModelNode;
typedef boost::shared_ptr<EclassModelNode> EclassModelNodePtr;

class EclassModelNode :
	public EntityNode,
	public Snappable
{
private:
	friend class EclassModel;

	EclassModel m_contained;

	AABB _localAABB;

private:
	// Constructor
	EclassModelNode(const IEntityClassPtr& eclass);
	// Copy Constructor
	EclassModelNode(const EclassModelNode& other);

public:
	static EclassModelNodePtr Create(const IEntityClassPtr& eclass);

	// Snappable implementation
	virtual void snapto(float snap);

	// Bounded implementation
	virtual const AABB& localAABB() const;

	scene::INodePtr clone() const;

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;
	void setRenderSystem(const RenderSystemPtr& renderSystem);

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

} // namespace
