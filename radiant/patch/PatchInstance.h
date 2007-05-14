#ifndef PATCHINSTANCE_H_
#define PATCHINSTANCE_H_

#include "Patch.h"
#include "PatchControlInstance.h"
#include "renderable.h"
#include "dragplanes.h"
#include <vector>

/* greebo: So what is a PatchInstance? 
 * 
 * A PatchInstance contains the actual patch as member variable and manages the selection and transformation 
 * operations of the patch.
 * 
 * As it manages the selected control vertices, it also takes care that these are rendered properly. 
 * All the render calls are coming in here and get passed/wrapped to the patch member accordingly.
 * 
 * Not only the ControlVertices are managed here, there are also DragPlanes available to transform the patch.
 * This is also managed in this PatchInstance class.  
 */

class PatchInstance :
	public Patch::Observer,
	public scene::Instance,
	public Selectable,
	public Renderable,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public PlaneSelectable,
	public LightCullable,
	public TransformModifier,
	public Bounded,
	public Cullable
{
	// The reference to the patch of this instance, the actual patch is "stored" in the PatchNode class
	Patch& m_patch;
	
	// The patch control instances
	typedef std::vector<PatchControlInstance> PatchControlInstances;
	PatchControlInstances m_ctrl_instances;

	// The attached selectable
	ObservedSelectable m_selectable;

	DragPlanes m_dragPlanes;

	// An array of renderable points
	mutable RenderablePointVector m_render_selected;
	
	// An internal AABB variable to calculate the bounding box of the selected components (has to be mutable) 
	mutable AABB m_aabb_component;

	static ShaderPtr m_state_selpoint;

	const LightList* m_lightList;
public:
	STRING_CONSTANT(Name, "PatchInstance");

	// Constructor and Destructor
	PatchInstance(const scene::Path& path, scene::Instance* parent, Patch& patch);
	~PatchInstance();

	void lightsChanged();
	typedef MemberCaller<PatchInstance, &PatchInstance::lightsChanged> LightsChangedCaller;

	// The callback function that gets called when the attached selectable gets changed
	void selectedChanged(const Selectable& selectable);
	typedef MemberCaller1<PatchInstance, const Selectable&, &PatchInstance::selectedChanged> SelectedChangedCaller;

	// greebo: This gets called by the ObservedSelectable as soon as its selection state changes 
	// (see ObservedSelectable and PatchControlInstance)
	void selectedChangedComponent(const Selectable& selectable);
	typedef MemberCaller1<PatchInstance, const Selectable&, &PatchInstance::selectedChangedComponent> SelectedChangedComponentCaller;

	// Return the contained patch of this instance
	Patch& getPatch();
	
	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const;
	
	// Bounded implementation
	virtual const AABB& localAABB() const;

	// Initialise/release the static member variables
	static void constructStatic();
	static void destroyStatic();

	void allocate(std::size_t size);

	// Set the selection status. As this is an ObservedSelectable, the onChanged callback is performed automatically.
	void setSelected(bool select);
	bool isSelected() const;
	void invertSelected();

	// greebo: Updates the internal render array m_render_selected, that contains all control vertices that should be
	// rendered as highlighted.
	void update_selected() const;

	// Check the GlobalFilterSystem to ensure patches should be rendered.
	bool isVisible() const;

	// Render functions, these make sure that all things get rendered properly. The calls are also passed on
	// to the contained patch <m_patch>
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;
	
	// greebo: Renders the selected components. This is called by the above two render functions
	void renderComponentsSelected(Renderer& renderer, const VolumeTest& volume) const;
  
  	// Renders the components of this patch instance, makes use of the Patch::render_component() method 
	void renderComponents(Renderer& renderer, const VolumeTest& volume) const;

	// Test the Patch instance for selection
	void testSelect(Selector& selector, SelectionTest& test);

	// Sets all Control Instances to selected = <selected>
  	void selectCtrl(bool selected);
  	
  	// Returns true if any of the patch components is selected
	bool isSelectedComponents() const;
	
	// Set the components (control points or dragplanes) selection to <select>
	void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode);
	
	// Create the axis aligned bounding box of the selected components
	const AABB& getSelectedComponentsBounds() const;

	// Tests the patch components on selection using the passed SelectionTest 
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	// Returns true if any of the Control Vertices is selected
	bool selectedVertices();

	// Transforms the patch components with the given transformation matrix
	void transformComponents(const Matrix4& matrix);

	// Check if the drag planes pass the given selection test (and select them of course and call the callback) 
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
  
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);

	// greebo: snaps all the _selected_ components to the grid (should be called "snapSelectedComponents")
	void snapComponents(float snap);

	void evaluateTransform();
  
  	// Apply the transformation to the patch
	void applyTransform();
	typedef MemberCaller<PatchInstance, &PatchInstance::applyTransform> ApplyTransformCaller;

	bool testLight(const RendererLight& light) const;
};

#endif /*PATCHINSTANCE_H_*/
