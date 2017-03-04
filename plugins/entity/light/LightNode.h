#pragma once

#include "ilightnode.h"

#include "Light.h"
#include "dragplanes.h"
#include "../VertexInstance.h"
#include "../EntityNode.h"

namespace entity
{

class LightNode;
typedef std::shared_ptr<LightNode> LightNodePtr;

class LightNode :
	public EntityNode,
	public ILightNode,
	public Snappable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public PlaneSelectable,
	public RendererLight
{
private:
	Light _light;

	// The (draggable) light center instance
	VertexInstance _lightCenterInstance;

	VertexInstance _lightTargetInstance;
	VertexInstanceRelative _lightRightInstance;
	VertexInstanceRelative _lightUpInstance;
	VertexInstance _lightStartInstance;
	VertexInstance _lightEndInstance;

	// dragplanes for lightresizing using mousedrag
    selection::DragPlanes _dragPlanes;

	// a temporary variable for calculating the AABB of all (selected) components
	mutable AABB m_aabb_component;

public:
	LightNode(const IEntityClassPtr& eclass);

private:
	LightNode(const LightNode& other);

public:
	static LightNodePtr Create(const IEntityClassPtr& eclass);

	// RenderEntity implementation
	virtual float getShaderParm(int parmNum) const override;

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	// override scene::Inode methods to deselect the child components
	virtual void onInsertIntoScene(scene::IMapRootNode& root) override;
	virtual void onRemoveFromScene(scene::IMapRootNode& root) override;

	// Snappable implementation
	virtual void snapto(float snap) override;

	/** greebo: Returns the AABB of the small diamond representation.
	 *	(use this to select the light against an AABB selectiontest like CompleteTall or similar).
	 */
	AABB getSelectAABB() override;

	/*greebo: This is a callback function that gets connected in the constructor
	* Don't know exactly what it does, but it seems to notify the shader cache that the light has moved or
	* something like that.*/
	void lightChanged();

	/* greebo: This snaps the components to the grid.
	 *
	 * Note: if none are selected, ALL the components are snapped to the grid (I hope this is intentional)
	 * This function can only be called in Selection::eVertex mode, so I assume that the user wants all components
	 * to be snapped.
	 *
	 * If one or more components is/are selected, ONLY those are snapped to the grid.
	 */
	void snapComponents(float snap) override;

	// PlaneSelectable implementation
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) override;
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) override;

	// Test the light volume for selection, this just passes the call on to the contained Light class
	void testSelect(Selector& selector, SelectionTest& test) override;

	// greebo: Returns true if drag planes or the light center is selected (both are components)
	bool isSelectedComponents() const override;
	// greebo: Selects/deselects all components, depending on the chosen componentmode
	void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) override;
	void invertSelectedComponents(SelectionSystem::EComponentMode mode) override;
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) override;

	/**
	 * greebo: This returns the AABB of all the selectable vertices. This method
	 * distinguishes between projected and point lights and stretches the AABB accordingly.
	 */
	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const override;

	scene::INodePtr clone() const override;

	void selectedChangedComponent(const ISelectable& selectable);

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

  	// Renders the components of this light instance
	void renderComponents(RenderableCollector& collector, const VolumeTest& volume) const override;

	// RendererLight implementation
    const Vector3& worldOrigin() const override;
    Matrix4 getLightTextureTransformation() const override;
    const ShaderPtr& getShader() const override;
	bool intersectsAABB(const AABB& other) const override;

	Vector3 getLightOrigin() const override;
	const Matrix4& rotation() const;

    // Returns the original "origin" value
    const Vector3& getUntransformedOrigin() override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
    void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

	// Override EntityNode::construct()
	void construct() override;

private:
	void renderInactiveComponents(RenderableCollector& collector, const VolumeTest& volume, const bool selected) const;

	void evaluateTransform();

}; // class LightNode

} // namespace entity
