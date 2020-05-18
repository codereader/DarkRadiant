#pragma once

#include "igroupnode.h"
#include "icurve.h"
#include "Doom3Group.h"
#include "irenderable.h"
#include "../NameKey.h"
#include "../curve/CurveEditInstance.h"
#include "../VertexInstance.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"
#include "../KeyObserverDelegate.h"

namespace entity 
{

class Doom3GroupNode;
typedef std::shared_ptr<Doom3GroupNode> Doom3GroupNodePtr;

class Doom3GroupNode :
	public EntityNode,
	public scene::GroupNode,
	public Snappable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public CurveNode
{
private:
	friend class Doom3Group;

	// The contained Doom3Group class
	Doom3Group _d3Group;

	CurveEditInstance _nurbsEditInstance;
	CurveEditInstance _catmullRomEditInstance;
	mutable AABB m_aabb_component;

	VertexInstance _originInstance;

private:
	// Constructor
	Doom3GroupNode(const IEntityClassPtr& eclass);
	// Private copy constructor, is invoked by clone()
	Doom3GroupNode(const Doom3GroupNode& other);

public:
	static Doom3GroupNodePtr Create(const IEntityClassPtr& eclass);

	virtual ~Doom3GroupNode();

	// CurveNode implementation
	virtual bool hasEmptyCurve() override;
	virtual void appendControlPoints(unsigned int numPoints) override;
	virtual void removeSelectedControlPoints() override;
	virtual void insertControlPointsAtSelected() override;
	virtual void convertCurveType() override;

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	/** greebo: Tests the contained Doom3Group for selection.
	 *
	 * Note: This can be successful in vertex mode only, func_statics do not use this method.
	 */
	void testSelect(Selector& selector, SelectionTest& test) override;

	// ComponentSelectionTestable implementation
	bool isSelectedComponents() const override;
	void setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode) override;
	void invertSelectedComponents(SelectionSystem::EComponentMode mode) override;
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) override;

	// override scene::Inode::onRemoveFromScene to deselect the child components
	virtual void onInsertIntoScene(scene::IMapRootNode& root) override;
	virtual void onRemoveFromScene(scene::IMapRootNode& root) override;

	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const override;

	// ComponentSnappable implementation
	void snapComponents(float snap) override;

	// Snappable implementation
	virtual void snapto(float snap) override;

	void selectionChangedComponent(const ISelectable& selectable);

	scene::INodePtr clone() const override;

	/** greebo: Call this right before map save to let the child
	 * brushes have their origin recalculated.
	 */
	void addOriginToChildren() override;
	void removeOriginFromChildren() override;

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	void renderComponents(RenderableCollector& collector, const VolumeTest& volume) const override;

	void transformComponents(const Matrix4& matrix);

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
    void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

	// Model Key changed signal
	void onModelKeyChanged(const std::string& value) override;

	// Override EntityNode::construct()
	virtual void construct() override;

private:
	void evaluateTransform();

};

} // namespace
