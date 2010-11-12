#ifndef SPEAKERNODE_H_
#define SPEAKERNODE_H_

#include "nameable.h"
#include "editable.h"
#include "inamespace.h"

#include "scenelib.h"
#include "transformlib.h"
#include "irenderable.h"
#include "selectionlib.h"
#include "dragplanes.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"

#include "Speaker.h"

namespace entity {

class SpeakerNode :
	public EntityNode,
	public Snappable,
	public SelectionTestable,
	public PlaneSelectable,
	public ComponentSelectionTestable
{
	friend class Speaker;

    // The Speaker class itself
	Speaker _speaker;

	// dragplanes for resizing using mousedrag
	DragPlanes _dragPlanes;

public:
	SpeakerNode(const IEntityClassPtr& eclass);
	SpeakerNode(const SpeakerNode& other);

	// Called after the constructor is done
	void construct();

	// Snappable implementation
	virtual void snapto(float snap);

	// EntityNode implementation
	virtual void refreshModel();

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// PlaneSelectable implementation
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);

	// ComponentSelectionTestable implementation
	bool isSelectedComponents() const;
	void setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode);
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test);

	scene::INodePtr clone() const;

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	void selectedChangedComponent(const Selectable& selectable);

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
	void _onTransformationChanged();

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
	void _applyTransformation();

private:
	void evaluateTransform();
};
typedef boost::shared_ptr<SpeakerNode> SpeakerNodePtr;

} // namespace entity

#endif /*SPEAKERNODE_H_*/
