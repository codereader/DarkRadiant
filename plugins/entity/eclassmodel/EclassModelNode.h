#ifndef ECLASSMODELNODE_H_
#define ECLASSMODELNODE_H_

#include "nameable.h"
#include "inamespace.h"
#include "modelskin.h"
#include "ientity.h"
#include "iselection.h"

#include "scenelib.h"
#include "scene/TraversableNodeSet.h"
#include "transformlib.h"
#include "selectionlib.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"
#include "../KeyObserverDelegate.h"

#include "EclassModel.h"

namespace entity {

class EclassModelNode :
	public EntityNode,
	public Snappable,
	public SelectionTestable
{
	friend class EclassModel;

	EclassModel m_contained;

	mutable bool _updateSkin;

	AABB _localAABB;

	KeyObserverDelegate _skinObserver;

public:
	// Constructor
	EclassModelNode(const IEntityClassPtr& eclass);
	// Copy Constructor
	EclassModelNode(const EclassModelNode& other);

	virtual ~EclassModelNode();

	// Snappable implementation
	virtual void snapto(float snap);

	// EntityNode implementation
	virtual void refreshModel();

	// Bounded implementation
	virtual const AABB& localAABB() const;

	scene::INodePtr clone() const;

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	// SelectionTestable
	void testSelect(Selector& selector, SelectionTest& test);

	void skinChanged(const std::string& value);

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
	void _onTransformationChanged();

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
	void _applyTransformation();

public:
	void construct();
	void destroy();
};
typedef boost::shared_ptr<EclassModelNode> EclassModelNodePtr;

} // namespace entity

#endif /*ECLASSMODELNODE_H_*/
