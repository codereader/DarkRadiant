#include "PatchNode.h"

#include "ifilter.h"
#include "iradiant.h"
#include "icounter.h"
#include "math/frustum.h"

// Construct a PatchNode with no arguments
PatchNode::PatchNode(bool patchDef3) :
	m_dragPlanes(SelectedChangedComponentCaller(*this)),
	_selectable(SelectedChangedCaller(*this)),
	m_render_selected(GL_POINTS),
	m_patch(*this, 
			EvaluateTransformCaller(*this), 
			Node::BoundsChangedCaller(*this)), // create the m_patch member with the node parameters
	m_importMap(m_patch),
	m_exportMap(m_patch),
	m_lightList(NULL)
{
	m_patch.m_patchDef3 = patchDef3;
	m_lightList = &GlobalRenderSystem().attach(*this);

	m_patch.m_lightsChanged = LightsChangedCaller(*this);

	Node::setTransformChangedCallback(LightsChangedCaller(*this));
}
  
// Copy Constructor
PatchNode::PatchNode(const PatchNode& other) :
	scene::Node(other),
	scene::Cloneable(other),
	Nameable(other),
	Snappable(other),
	MapImporter(other),
	MapExporter(other),
	IPatchNode(other),
	Selectable(other),
	SelectionTestable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	PlaneSelectable(other),
	LightCullable(other),
	Renderable(other),
	Bounded(other),
	Transformable(other),
	m_dragPlanes(SelectedChangedComponentCaller(*this)),
	_selectable(SelectedChangedCaller(*this)),
	m_render_selected(GL_POINTS),
	m_patch(other.m_patch, *this, EvaluateTransformCaller(*this), 
		    Node::BoundsChangedCaller(*this)), // create the patch out of the <other> one
	m_importMap(m_patch),
	m_exportMap(m_patch),
	m_lightList(NULL)
{
	m_lightList = &GlobalRenderSystem().attach(*this);

	m_patch.m_lightsChanged = LightsChangedCaller(*this);

	Node::setTransformChangedCallback(LightsChangedCaller(*this));
}

PatchNode::~PatchNode()
{
	GlobalRenderSystem().detach(*this);
}

void PatchNode::allocate(std::size_t size) {
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

const AABB& PatchNode::localAABB() const {
	return m_patch.localAABB();
}

std::string PatchNode::name() const {
	return "Patch";
}

Patch& PatchNode::getPatchInternal() {
	return m_patch;
}

IPatch& PatchNode::getPatch() {
	return m_patch;
}

void PatchNode::lightsChanged() {
	m_lightList->lightsChanged();
}

// Snappable implementation
void PatchNode::snapto(float snap) {
	m_patch.snapto(snap);
}

// MapImporter implementation
bool PatchNode::importTokens(parser::DefTokeniser& tokeniser) {
	return m_importMap.importTokens(tokeniser);
}

// MapExporter implementation
void PatchNode::exportTokens(std::ostream& os) const {
	m_exportMap.exportTokens(os);
}

bool PatchNode::selectedVertices() {
	// Cycle through all the instances and return true as soon as the first selected one is found
	for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if (i->m_selectable.isSelected()) {
			return true;
		}
	}
    return false;
}

void PatchNode::snapComponents(float snap) {
	// Are there any selected vertices
	if (selectedVertices()) {
		// Tell the patch to save the current undo state
		m_patch.undoSave();
		
		// Cycle through all the selected control instances and snap them to the grid
		for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
			if(i->m_selectable.isSelected()) {
				i->snapto(snap);
			}
		}
		// Tell the patch that control points have changed
		m_patch.controlPointsChanged();
	}
}

// Test the Patch instance for selection
void PatchNode::testSelect(Selector& selector, SelectionTest& test) {
	// Do not select patch if it is filtered
	if (!isVisible())
		return;
	
    test.BeginMesh(localToWorld(), true);
    // Pass the selection test call to the patch
    m_patch.testSelect(selector, test);
}

void PatchNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());

	// Check if the drag planes pass the selection test 	
	m_dragPlanes.selectPlanes(m_patch.localAABB(), selector, test, selectedPlaneCallback);
}

void PatchNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	// Pass the call to the according dragplane member method
	m_dragPlanes.selectReversedPlanes(m_patch.localAABB(), selector, selectedPlanes);
}

void PatchNode::selectCtrl(bool selected) {
	// Cycle through all ControlInstances and set them to <select> 
	for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		i->m_selectable.setSelected(selected);
	}
}

bool PatchNode::isSelectedComponents() const {
	// Cycle through all ControlInstances and return true on the first selected one that is found
	for (PatchControlInstances::const_iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if (i->m_selectable.isSelected()) {
			return true;
		}
	}
	return false;
}

void PatchNode::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) {
	// If we are in vertex edit mode, set the control vertices to <select>
	if (mode == SelectionSystem::eVertex) {
		selectCtrl(select);
	}
	// If we are in vertex edit mode, set the dragplanes to <select> 
	else if (mode == SelectionSystem::eFace) {
		m_dragPlanes.setSelected(select);
	}
}

void PatchNode::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	test.BeginMesh(localToWorld());

	// Only react to eVertex selection mode
	switch(mode) {
		case SelectionSystem::eVertex: {
			// Cycle through all the control instances and test them for selection
			for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
				i->testSelect(selector, test);
			}
		}
		break;
		default:
		break;
	}
}

const AABB& PatchNode::getSelectedComponentsBounds() const {
	// Create a new axis aligned bounding box
	m_aabb_component = AABB();

	// Cycle through all the instances and extend the bounding box by using the selected control points
	for (PatchControlInstances::const_iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if (i->m_selectable.isSelected()) {
			m_aabb_component.includePoint(i->m_ctrl->vertex);
		}
	}

	return m_aabb_component;
}

bool PatchNode::isVisible() const {
	return visible() && m_patch.getState()->getMaterial()->isVisible();
}

void PatchNode::setSelected(bool select) {
	_selectable.setSelected(select);
}

bool PatchNode::isSelected() const {
	return _selectable.isSelected();
}

void PatchNode::invertSelected() {
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent) {
		// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
		PatchControlIter ctrl = m_patch.getControlPointsTransformed().begin();
		
		for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i, ++ctrl)
		{
			i->m_selectable.invertSelected();
		}
	}
	else {
		// Invert the selection of the patch itself
		setSelected(!isSelected());
	}
}

void PatchNode::selectedChanged(const Selectable& selectable) {
	GlobalSelectionSystem().onSelectedChanged(Node::getSelf(), selectable);

	// TODO? instance->selectedChanged();
}

void PatchNode::selectedChangedComponent(const Selectable& selectable) {
	// Notify the selection system that this PatchNode was selected. The RadiantSelectionSystem adds
	// this to its internal list of selected nodes. 
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

// Clones this node, allocates a new Node on the heap and passes itself to the constructor of the new node
scene::INodePtr PatchNode::clone() const {
	return scene::INodePtr(new PatchNode(*this));
}

void PatchNode::onInsertIntoScene()
{
	m_patch.instanceAttach(scene::findMapFile(getSelf()));
	GlobalCounters().getCounter(counterPatches).increment();

	Node::onInsertIntoScene();
}

void PatchNode::onRemoveFromScene()
{
	// De-select this node
	setSelected(false);

	// De-select all child components as well
	setSelectedComponents(false, SelectionSystem::eVertex);

	GlobalCounters().getCounter(counterPatches).decrement();

	m_patch.instanceDetach(scene::findMapFile(getSelf()));

	Node::onRemoveFromScene();
}

bool PatchNode::testLight(const RendererLight& light) const {
	return light.testAABB(worldAABB());
}

void PatchNode::constructStatic() {
	m_state_selpoint = GlobalRenderSystem().capture("$SELPOINT");
}

void PatchNode::destroyStatic() {
	m_state_selpoint = ShaderPtr();
}

void PatchNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const {
	// If not visible, there's nothing to do for us
	if (!isVisible())
		return;

	// greebo: Don't know yet, what evaluateTransform() is really doing
	const_cast<Patch&>(m_patch).evaluateTransform();
	collector.setLights(*m_lightList);
	
	// Pass the call to the patch instance, it adds the renderable
	m_patch.render_solid(collector, volume, localToWorld());

	// Render the selected components
	renderComponentsSelected(collector, volume);
}

void PatchNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const {
	// If not visible, there's nothing to do for us
	if (!isVisible())
		return;
	
	// greebo: Don't know yet, what evaluateTransform() is really doing
	const_cast<Patch&>(m_patch).evaluateTransform();
	
	// Pass the call to the patch instance, it adds the renderable
	m_patch.render_wireframe(collector, volume, localToWorld());
	
	// Render the selected components
	renderComponentsSelected(collector, volume);
}

// Renders the components of this patch instance, makes use of the Patch::render_component() method 
void PatchNode::renderComponents(RenderableCollector& collector, const VolumeTest& volume) const {
	if (!isVisible())
		return;

	// greebo: Don't know yet, what evaluateTransform() is really doing
	const_cast<Patch&>(m_patch).evaluateTransform();
		
	// Only render the components, if we are in the according ComponentMode
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex) {
		// Call the method of the patch itself
		m_patch.render_component(collector, volume, localToWorld());
	}
}

void PatchNode::update_selected() const {
	// Clear the renderable point vector that represents the selection
	m_render_selected.clear();
	
	// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
	PatchControlConstIter ctrl = m_patch.getControlPointsTransformed().begin();
	
	for (PatchControlInstances::const_iterator i = m_ctrl_instances.begin(); 
		 i != m_ctrl_instances.end(); ++i, ++ctrl)
	{
		if (i->m_selectable.isSelected()) {
			const Colour4b colour_selected(0, 0, 0, 255);
			// Add this patch control instance to the render list
			m_render_selected.push_back(PointVertex(reinterpret_cast<const Vertex3f&>(ctrl->vertex), colour_selected));
		}
	}
}

void PatchNode::renderComponentsSelected(RenderableCollector& collector, const VolumeTest& volume) const {
	// If not visible, there's nothing to do for us
	if (!isVisible())
		return;

	// greebo: Don't know yet, what evaluateTransform() is really doing
	const_cast<Patch&>(m_patch).evaluateTransform();
	
	// Rebuild the array of selected control vertices
	update_selected();
	
	// If there are any selected components, add them to the collector 
	if (!m_render_selected.empty()) {
		collector.Highlight(RenderableCollector::ePrimitive, false);
		collector.SetState(PatchNode::m_state_selpoint, RenderableCollector::eWireframeOnly);
		collector.SetState(PatchNode::m_state_selpoint, RenderableCollector::eFullMaterials);
		collector.addRenderable(m_render_selected, localToWorld());
	}
}

void PatchNode::evaluateTransform()
{
	Matrix4 matrix = calculateTransform();

	// Avoid transform calls when an identity matrix is passed,
	// this equality check is cheaper than all the stuff going on in transform().
	if (matrix == Matrix4::getIdentity()) return;

	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_patch.transform(matrix);
	}
	else
	{
		transformComponents(matrix);
	}
}

void PatchNode::transformComponents(const Matrix4& matrix) {
	// Are there any selected vertices?
	if (selectedVertices())
	{
		// Set the iterator to the start of the (transformed) control points array 
		PatchControlIter ctrl = m_patch.getControlPointsTransformed().begin();
		
		// Cycle through the patch control instances and transform the selected ones
		// greebo: Have to investigate this further, why there are actually two iterators needed  
		for (PatchNode::PatchControlInstances::iterator i = m_ctrl_instances.begin(); 
			 i != m_ctrl_instances.end(); ++i, ++ctrl)
		{
			if (i->m_selectable.isSelected())
			{
				matrix4_transform_point(matrix, ctrl->vertex);
			}
		}

		// mark this patch transform as dirty
		m_patch.transformChanged();
	}

	// Also, check if there are any drag planes selected
	// this should only be true when the transform is a pure translation.
	if (m_dragPlanes.isSelected())
	{ 
		m_patch.transform(m_dragPlanes.evaluateTransform(matrix.t().getVector3()));
	}
}

void PatchNode::_onTransformationChanged()
{
	m_patch.transformChanged();
}

void PatchNode::_applyTransformation()
{
	// First, revert the changes, then recalculate the transformation and then freeze the changes
	m_patch.revertTransform();
	evaluateTransform();
	m_patch.freezeTransform();
}

// Initialise the shader member variable
ShaderPtr PatchNode::m_state_selpoint;
