#ifndef LIGHTNODE_H_
#define LIGHTNODE_H_

#include "scenelib.h"

#include "Light.h"
#include "dragplanes.h"
#include "../VertexInstance.h"
#include "../target/TargetableNode.h"
#include "../EntityNode.h"

namespace entity {

class LightNode :
	public EntityNode,
	public SelectableNode,
	public scene::Cloneable,
	public Nameable,
	public Snappable,
	public Editable,
	public TransformNode,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public PlaneSelectable,
	public Renderable,
	public Bounded,
	public TransformModifier,
	public RendererLight,
	public scene::SelectableLight,
	public TargetableNode
{
	friend class Light;

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
	LightNode(IEntityClassPtr eclass);
	LightNode(const LightNode& other);

	virtual ~LightNode();
	
	// EntityNode implementation
	virtual Entity& getEntity();
	virtual void refreshModel();

	// Bounded implementation
	virtual const AABB& localAABB() const;
	
	// override scene::Inode::onRemoveFromScene to deselect the child components
	virtual void onRemoveFromScene();

	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

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

	// scene::Instantiable implementation
	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);

	void selectedChangedComponent(const Selectable& selectable);
	typedef MemberCaller1<LightNode, const Selectable&, &LightNode::selectedChangedComponent> SelectedChangedComponentCaller;

	// Nameable implementation
	virtual std::string name() const;
	virtual void attach(const NameCallback& callback);
	virtual void detach(const NameCallback& callback);

	// Renderable implementation
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;  
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;
  	// Renders the components of this light instance 
	void renderComponents(Renderer& renderer, const VolumeTest& volume) const;

	void evaluateTransform();
	typedef MemberCaller<LightNode, &LightNode::evaluateTransform> EvaluateTransformCaller;
	void applyTransform();
	typedef MemberCaller<LightNode, &LightNode::applyTransform> ApplyTransformCaller;

    /* RendererLight implementation */
    Vector3 worldOrigin() const;
	ShaderPtr getShader() const;
	const AABB& aabb() const;
	bool testAABB(const AABB& other) const;
	
	const Vector3& offset() const;
	const Matrix4& rotation() const;
	const Vector3& colour() const;

	bool isProjected() const;
	const Matrix4& projection() const;

private:
	void renderInactiveComponents(Renderer& renderer, const VolumeTest& volume, const bool selected) const;	

}; // class LightNode

} // namespace entity

#endif /*LIGHTNODE_H_*/
