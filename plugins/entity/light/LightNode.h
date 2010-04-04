#ifndef LIGHTNODE_H_
#define LIGHTNODE_H_

#include "scenelib.h"

#include "Light.h"
#include "dragplanes.h"
#include "../VertexInstance.h"
#include "../EntityNode.h"

namespace entity {

class LightNode :
	public EntityNode,
	public Snappable,
	public Editable,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public PlaneSelectable,
	public RendererLight,
	public scene::SelectableLight
{
	Light _light;

	// The (draggable) light center instance
	VertexInstance _lightCenterInstance;
	
	VertexInstance _lightTargetInstance;
	VertexInstanceRelative _lightRightInstance;
	VertexInstanceRelative _lightUpInstance;
	VertexInstance _lightStartInstance;
	VertexInstance _lightEndInstance;
	
	// dragplanes for lightresizing using mousedrag
	DragPlanes m_dragPlanes;
	// a temporary variable for calculating the AABB of all (selected) components
	mutable AABB m_aabb_component;
	
public:
	LightNode(const IEntityClassPtr& eclass);
	LightNode(const LightNode& other);

	void construct();

	virtual ~LightNode();
	
	// EntityNode implementation
	virtual void refreshModel();

	// Bounded implementation
	virtual const AABB& localAABB() const;
	
	// override scene::Inode methods to deselect the child components
	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

	// Editable implementation
	virtual const Matrix4& getLocalPivot() const;

	// Snappable implementation
	virtual void snapto(float snap);

	/** greebo: Returns the AABB of the small diamond representation. 
	 *	(use this to select the light against an AABB selectiontest like CompleteTall or similar).
	 */
	AABB getSelectAABB();

	/*greebo: This is a callback function that gets connected in the constructor
	* Don't know exactly what it does, but it seems to notify the shader cache that the light has moved or
	* something like that.*/ 
	void lightChanged();
	typedef MemberCaller<LightNode, &LightNode::lightChanged> LightChangedCaller;

	/* greebo: This snaps the components to the grid.
	 * 
	 * Note: if none are selected, ALL the components are snapped to the grid (I hope this is intentional)
	 * This function can only be called in Selection::eVertex mode, so I assume that the user wants all components
	 * to be snapped.
	 * 
	 * If one or more components is/are selected, ONLY those are snapped to the grid.  
	 */
	void snapComponents(float snap);

	// PlaneSelectable implementation
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);

	// Test the light volume for selection, this just passes the call on to the contained Light class
	void testSelect(Selector& selector, SelectionTest& test);

	// greebo: Returns true if drag planes or the light center is selected (both are components)
	bool isSelectedComponents() const;
	// greebo: Selects/deselects all components, depending on the chosen componentmode
	void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode);
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	/** 
	 * greebo: This returns the AABB of all the selectable vertices. This method
	 * distinguishes between projected and point lights and stretches the AABB accordingly.
	 */
	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const;

	scene::INodePtr clone() const;

	void selectedChangedComponent(const Selectable& selectable);

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;  
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;
  	// Renders the components of this light instance 
	void renderComponents(RenderableCollector& collector, const VolumeTest& volume) const;

	// RendererLight implementation
    Vector3 worldOrigin() const;
    Matrix4 getLightTextureTransformation() const;
	ShaderPtr getShader() const;
	bool testAABB(const AABB& other) const;
	
	Vector3 getLightOrigin() const;
	const Matrix4& rotation() const;
	const Vector3& colour() const;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
	void _onTransformationChanged();

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
	void _applyTransformation();

private:
	void renderInactiveComponents(RenderableCollector& collector, const VolumeTest& volume, const bool selected) const;	

	void evaluateTransform();

}; // class LightNode
typedef boost::shared_ptr<LightNode> LightNodePtr;

} // namespace entity

#endif /*LIGHTNODE_H_*/
