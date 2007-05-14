#include "PatchInstance.h"

#include "ifilter.h"
#include "math/frustum.h"

// Initialise the shader member variable
ShaderPtr PatchInstance::m_state_selpoint;

// Constructor: Create an PatchInstance with the given <patch> and the scene elements <path> and <parent>
PatchInstance::PatchInstance(const scene::Path& path, scene::Instance* parent, Patch& patch) :
	Instance(path, parent),
	TransformModifier(Patch::TransformChangedCaller(patch), ApplyTransformCaller(*this)),
	m_patch(patch),
	m_selectable(SelectedChangedCaller(*this)),	// Connect the onChange callback and pass self as the observed Selectable
	m_dragPlanes(SelectedChangedComponentCaller(*this)),
	m_render_selected(GL_POINTS)
{
	// Attach the path and self to the contained patch
	m_patch.instanceAttach(Instance::path());
	m_patch.attach(this);

	m_lightList = &GlobalShaderCache().attach(*this);
	m_patch.m_lightsChanged = LightsChangedCaller(*this);

	Instance::setTransformChangedCallback(LightsChangedCaller(*this));
}
  
// Destructor
PatchInstance::~PatchInstance() {
	Instance::setTransformChangedCallback(Callback());

	m_patch.m_lightsChanged = Callback();
	GlobalShaderCache().detach(*this);

	m_patch.detach(this);
	m_patch.instanceDetach(Instance::path());
}

void PatchInstance::lightsChanged() {
	m_lightList->lightsChanged();
}

void PatchInstance::selectedChanged(const Selectable& selectable) {
	GlobalSelectionSystem().getObserver(SelectionSystem::ePrimitive)(selectable);
	GlobalSelectionSystem().onSelectedChanged(*this, selectable);

	Instance::selectedChanged();
}

void PatchInstance::selectedChangedComponent(const Selectable& selectable) {
	// greebo: This line calls the observer function provided by the SelectionSystem (usually a CountSelected class
	// that keeps track of the selected instances)
	GlobalSelectionSystem().getObserver(SelectionSystem::eComponent)(selectable);
	// Notify the selection system that this PatchInstance was selected. The RadiantSelectionSystem adds
	// this to its internal list of selected instances. 
	// The pointer _ *this _ is therefore passed as a scene::instance pointer 
	GlobalSelectionSystem().onComponentSelection(*this, selectable);
}

Patch& PatchInstance::getPatch() {
	return m_patch;
}

const AABB& PatchInstance::localAABB() const {
	return m_patch.localAABB();
}

VolumeIntersectionValue PatchInstance::intersectVolume(
	const VolumeTest& test, const Matrix4& localToWorld) const
{
	return m_patch.intersectVolume(test, localToWorld);
}

void PatchInstance::constructStatic() {
	m_state_selpoint = GlobalShaderCache().capture("$SELPOINT");
}

void PatchInstance::destroyStatic() {
	m_state_selpoint = ShaderPtr();
}
  
void PatchInstance::allocate(std::size_t size) {
	// Clear the control instance vector and reserve <size> memory
	m_ctrl_instances.clear();
	m_ctrl_instances.reserve(size);
	
	// greebo: Cycle through the patch's control vertices and add them as PatchControlInstance to the vector
	// The PatchControlInstance constructor takes a pointer to a PatchControl and the SelectionChanged callback
	// The passed callback points back to this class (the member method selectedChangedComponent() is called).  
	for(PatchControlIter i = m_patch.begin(); i != m_patch.end(); ++i) {
		m_ctrl_instances.push_back(PatchControlInstance(&(*i), SelectedChangedComponentCaller(*this)));
	}
}

void PatchInstance::setSelected(bool select) {
	m_selectable.setSelected(select);
}

bool PatchInstance::isSelected() const {
	return m_selectable.isSelected();
}

void PatchInstance::invertSelected() {
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent) {
		// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
		PatchControlIter ctrl = m_patch.getControlPointsTransformed().begin();
		
		for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i, ++ctrl) {
			i->m_selectable.invertSelected();
		}
	}
	else {
		// Invert the selection of the patch itself
		setSelected(!isSelected());
	}
}

void PatchInstance::update_selected() const {
	// Clear the renderable point vector that represents the selection
	m_render_selected.clear();
	
	// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
	PatchControlIter ctrl = m_patch.getControlPointsTransformed().begin();
	
	for (PatchControlInstances::const_iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i, ++ctrl) {
		if((*i).m_selectable.isSelected()) {
			const Colour4b colour_selected(0, 0, 0, 255);
			// Add this patch control instance to the render list
			m_render_selected.push_back(PointVertex(reinterpret_cast<Vertex3f&>((*ctrl).m_vertex), colour_selected));
		}
	}
}

void PatchInstance::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	// If not visible, there's nothing to do for us
	if (!isVisible())
		return;
	
	// greebo: Don't know yet, what evaluateTransform() is really doing
	m_patch.evaluateTransform();
	renderer.setLights(*m_lightList);
	
	// Pass the call to the patch instance, it adds the renderable
	m_patch.render_solid(renderer, volume, localToWorld());

	// Render the selected components
	renderComponentsSelected(renderer, volume);
}

void PatchInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	// If not visible, there's nothing to do for us
	if (!isVisible())
		return;
	
	// greebo: Don't know yet, what evaluateTransform() is really doing
	m_patch.evaluateTransform();
	
	// Pass the call to the patch instance, it adds the renderable
	m_patch.render_wireframe(renderer, volume, localToWorld());
	
	// Render the selected components
	renderComponentsSelected(renderer, volume);
}

// Renders the components of this patch instance, makes use of the Patch::render_component() method 
void PatchInstance::renderComponents(Renderer& renderer, const VolumeTest& volume) const {
	if (!isVisible())
		return;

	// greebo: Don't know yet, what evaluateTransform() is really doing
	m_patch.evaluateTransform();
		
	// Only render the components, if we are in the according ComponentMode
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex) {
		// Call the method of the patch itself
		m_patch.render_component(renderer, volume, localToWorld());
	}
}

void PatchInstance::renderComponentsSelected(Renderer& renderer, const VolumeTest& volume) const {
	// If not visible, there's nothing to do for us
	if (!isVisible())
		return;

	// greebo: Don't know yet, what evaluateTransform() is really doing
	m_patch.evaluateTransform();
	
	// Rebuild the array of selected control vertices
	update_selected();
	
	// If there are any selected components, add them to the renderer 
	if (!m_render_selected.empty()) {
		renderer.Highlight(Renderer::ePrimitive, false);
		renderer.SetState(m_state_selpoint, Renderer::eWireframeOnly);
		renderer.SetState(m_state_selpoint, Renderer::eFullMaterials);
		renderer.addRenderable(m_render_selected, localToWorld());
	}
}

// Test the Patch instance for selection
void PatchInstance::testSelect(Selector& selector, SelectionTest& test) {
	// Do not select patch if it is filtered
	if (!isVisible())
		return;
    test.BeginMesh(localToWorld(), true);
    // Pass the selection test call to the patch
    m_patch.testSelect(selector, test);
}

// Check the GlobalFilterSystem to ensure patches should be rendered.
bool PatchInstance::isVisible() const {
	return GlobalFilterSystem().isVisible("object", "patch");	
}

void PatchInstance::selectCtrl(bool selected) {
	// Cycle through all ControlInstances and set them to <select> 
	for(PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		(*i).m_selectable.setSelected(selected);
	}
}

bool PatchInstance::isSelectedComponents() const {
	// Cycle through all ControlInstances and return true on the first selected one that is found
	for(PatchControlInstances::const_iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if((*i).m_selectable.isSelected()) {
			return true;
		}
	}
	return false;
}

void PatchInstance::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) {
	// If we are in vertex edit mode, set the control vertices to <select>
	if (mode == SelectionSystem::eVertex) {
		selectCtrl(select);
	}
	// If we are in vertex edit mode, set the dragplanes to <select> 
	else if(mode == SelectionSystem::eFace) {
		m_dragPlanes.setSelected(select);
	}
}

const AABB& PatchInstance::getSelectedComponentsBounds() const {
	// Create a new axis aligned bounding box
	m_aabb_component = AABB();

	// Cycle through all the instances and extend the bounding box by using the selected control points
	for (PatchControlInstances::const_iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if ((*i).m_selectable.isSelected()) {
			m_aabb_component.includePoint(i->m_ctrl->m_vertex);
		}
	}

	return m_aabb_component;
}

void PatchInstance::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	test.BeginMesh(localToWorld());

	// Only react to eVertex selection mode
	switch(mode) {
		case SelectionSystem::eVertex: {
			// Cycle through all the control instances and test them for selection
			for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
				(*i).testSelect(selector, test);
			}
		}
		break;
		default:
		break;
	}
}

bool PatchInstance::selectedVertices() {
	// Cycle through all the instances and return true as soon as the first selected one is found
	for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if ((*i).m_selectable.isSelected()) {
			return true;
		}
	}
    return false;
}

void PatchInstance::transformComponents(const Matrix4& matrix) {
	// Are there any selected vertices?
	if (selectedVertices()) {
		// Set the iterator to the start of the (transformed) control points array 
		PatchControlIter ctrl = m_patch.getControlPointsTransformed().begin();
		
		// Cycle through the patch control instances and transform the selected ones
		// greebo: Have to investigate this further, why there are actually two iterators needed  
		for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i, ++ctrl) {
			if ((*i).m_selectable.isSelected()) {
				matrix4_transform_point(matrix, (*ctrl).m_vertex);
			}
		}
		m_patch.UpdateCachedData();
	}

	// Also, check if there are any drag planes selected
	if (m_dragPlanes.isSelected()) { // this should only be true when the transform is a pure translation.
		m_patch.transform(m_dragPlanes.evaluateTransform(matrix.t().getVector3()));
	}
}

void PatchInstance::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());

	// Check if the drag planes pass the selection test 	
	m_dragPlanes.selectPlanes(m_patch.localAABB(), selector, test, selectedPlaneCallback);
}

void PatchInstance::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	// Pass the call to the according dragplane member method
	m_dragPlanes.selectReversedPlanes(m_patch.localAABB(), selector, selectedPlanes);
}

void PatchInstance::snapComponents(float snap) {
	// Are there any selected vertices
	if (selectedVertices()) {
		// Tell the patch to save the current undo state
		m_patch.undoSave();
		
		// Cycle through all the selected control instances and snap them to the grid
		for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
			if((*i).m_selectable.isSelected()) {
				(*i).snapto(snap);
			}
		}
		// Tell the patch that control points have changed
		m_patch.controlPointsChanged();
	}
}

void PatchInstance::evaluateTransform() {
	Matrix4 matrix(calculateTransform());

	if (getType() == TRANSFORM_PRIMITIVE) {
		m_patch.transform(matrix);
	}
	else {
		transformComponents(matrix);
	}
}

void PatchInstance::applyTransform() {
	// First, revert the changes, then recalculate the transformation and then freeze the changes
	// greebo: Don't know why this has to be done this way (seems a bit weird to me, but perhaps this has its reason)
	m_patch.revertTransform();
	evaluateTransform();
	m_patch.freezeTransform();
}

bool PatchInstance::testLight(const RendererLight& light) const {
	return light.testAABB(worldAABB());
}
